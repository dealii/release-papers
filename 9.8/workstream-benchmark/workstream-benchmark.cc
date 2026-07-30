/* ------------------------------------------------------------------------
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Copyright (C) 2026 by the deal.II authors
 *
 * This file is part of the deal.II library.
 *
 * Part of the source code is dual licensed under Apache-2.0 WITH
 * LLVM-exception OR LGPL-2.1-or-later. Detailed license information
 * governing the source code and code contributions can be found in
 * LICENSE.md and CONTRIBUTING.md at the top level directory of deal.II.
 *
 * ------------------------------------------------------------------------
 */

// WorkStream benchmark comparing deal.II releases and threading backends.
//
// Times a single WorkStream::run() cell-assembly sweep -- a Poisson-like
// stiffness matrix and right hand side, FE_Q on a hypercube -- over FE
// degrees, global refinements, thread counts and chunk sizes, in the colored
// or the non-colored WorkStream mode.
//
//   ./workstream-benchmark input.prm > run.log 2>&1
//
// writes <run-id>_raw.csv (one row per WorkStream::run() call) and
// <run-id>_summary.csv (statistics over the timed calls).

#include <deal.II/base/config.h>

#include <deal.II/base/exceptions.h>
#include <deal.II/base/graph_coloring.h>
#include <deal.II/base/multithread_info.h>
#include <deal.II/base/parameter_handler.h>
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/revision.h>
#include <deal.II/base/work_stream.h>

#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_values.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria.h>

#include <deal.II/lac/dynamic_sparsity_pattern.h>
#include <deal.II/lac/full_matrix.h>
#include <deal.II/lac/sparse_matrix.h>
#include <deal.II/lac/vector.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <vector>


namespace WorkStreamReleaseBenchmark
{
  using namespace dealii;


  // Taskflow entered work_stream.h in 9.7, and takes precedence from there on.
  std::string
  backend_name()
  {
#if defined(DEAL_II_WITH_TASKFLOW) && DEAL_II_VERSION_GTE(9, 7, 0)
    return "taskflow";
#elif defined(DEAL_II_WITH_TBB)
    return "tbb";
#else
    return "sequential";
#endif
  }


  std::string
  backend_version()
  {
#if defined(DEAL_II_WITH_TASKFLOW) && DEAL_II_VERSION_GTE(9, 7, 0)
    return std::to_string(TF_VERSION / 100000) + '.' +
           std::to_string(TF_VERSION / 100 % 1000) + '.' +
           std::to_string(TF_VERSION % 100);
#elif defined(DEAL_II_WITH_TBB)
    return std::to_string(TBB_INTERFACE_VERSION);
#else
    return "-";
#endif
  }


  std::string
  platform_tag()
  {
#if defined(__APPLE__)
    const std::string operating_system = "macos";
#elif defined(__linux__)
    const std::string operating_system = "linux";
#else
    const std::string operating_system = "unknown-os";
#endif

#if defined(__aarch64__)
    const std::string architecture = "arm64";
#elif defined(__x86_64__)
    const std::string architecture = "x86_64";
#else
    const std::string architecture = "unknown-arch";
#endif

    return operating_system + '-' + architecture;
  }


  std::string
  compiler_description()
  {
#if defined(__clang__)
    return std::string("clang ") + __clang_version__;
#elif defined(__GNUC__)
    return std::string("gcc ") + __VERSION__;
#else
    return "unknown compiler";
#endif
  }


  std::string
  formatted_local_time(const std::time_t now, const char *const format)
  {
    std::tm local_time;
    localtime_r(&now, &local_time);

    std::ostringstream output;
    output << std::put_time(&local_time, format);
    return output.str();
  }


  double
  average(const std::vector<double> &values)
  {
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
  }


  enum class WorkStreamMode
  {
    colored,
    non_colored
  };


  std::string
  to_string(const WorkStreamMode mode)
  {
    return (mode == WorkStreamMode::colored ? "colored" : "non-colored");
  }


  struct BenchmarkParameters
  {
    void
    declare_parameters(ParameterHandler &prm);

    void
    parse_input_file(const std::string &input_file_name);

    unsigned int              dimension            = 2;
    std::string               mode_selection       = "colored";
    std::vector<unsigned int> thread_counts        = {1};
    std::vector<unsigned int> fe_degrees           = {1};
    std::vector<unsigned int> global_refinements   = {4};
    std::vector<unsigned int> chunk_sizes          = {8};
    unsigned int              n_warmup_repetitions = 2;
    unsigned int              n_timed_repetitions  = 10;
    double                    cooldown_seconds     = 0.0;
    std::string               output_prefix = "workstream-benchmark";

    std::vector<WorkStreamMode> modes;
    std::string                 effective_parameters;
  };


  void
  BenchmarkParameters::declare_parameters(ParameterHandler &prm)
  {
    prm.add_parameter("Dimension",
                      dimension,
                      "Space dimension.",
                      Patterns::Integer(2, 3));

    prm.add_parameter("WorkStream mode",
                      mode_selection,
                      "WorkStream algorithm: colored|non-colored|both.",
                      Patterns::Selection("colored|non-colored|both"));

    prm.add_parameter("Thread counts",
                      thread_counts,
                      "Thread counts to sweep. 1 thread is always measured, "
                      "as the speedup baseline.",
                      Patterns::List(Patterns::Integer(1), 1));

    prm.add_parameter("FE degrees",
                      fe_degrees,
                      "FE_Q degrees to sweep.",
                      Patterns::List(Patterns::Integer(1), 1));

    prm.add_parameter("Global refinements",
                      global_refinements,
                      "Global refinements of the hypercube to sweep.",
                      Patterns::List(Patterns::Integer(0), 1));

    prm.add_parameter("Chunk sizes",
                      chunk_sizes,
                      "The chunk_size values to sweep:\n"
                      "- colored, 9.7: cells per task, fixed task count\n"
                      "- colored, 9.8: tf::GuidedPartitioner chunk size\n"
                      "- colored, TBB: blocked_range grainsize, with "
                      "auto_partitioner\n"
                      "- non-colored, 9.7 and 9.8: cells per worker task, "
                      "copiers serialized\n"
                      "- non-colored, TBB: cells per parallel_pipeline item",
                      Patterns::List(Patterns::Integer(1), 1));

    prm.add_parameter("Warm-up repetitions",
                      n_warmup_repetitions,
                      "Untimed WorkStream::run() calls opening each "
                      "configuration, before the timed repetitions.",
                      Patterns::Integer(0));

    prm.add_parameter("Timed repetitions",
                      n_timed_repetitions,
                      "Timed WorkStream::run() calls per configuration.",
                      Patterns::Integer(1));

    prm.add_parameter("Cooldown seconds",
                      cooldown_seconds,
                      "Sleep opening each configuration, before warm-up.",
                      Patterns::Double(0.0));

    prm.add_parameter("Output prefix",
                      output_prefix,
                      "Prefix of the run id and the CSV file names.",
                      Patterns::Anything());
  }


  void
  BenchmarkParameters::parse_input_file(const std::string &input_file_name)
  {
    ParameterHandler prm;
    declare_parameters(prm);
    prm.parse_input(input_file_name);

    if (mode_selection == "colored")
      modes = {WorkStreamMode::colored};
    else if (mode_selection == "non-colored")
      modes = {WorkStreamMode::non_colored};
    else
      modes = {WorkStreamMode::colored, WorkStreamMode::non_colored};

    std::ostringstream effective;
    prm.print_parameters(effective,
                         ParameterHandler::PRM |
                           ParameterHandler::KeepDeclarationOrder);
    effective_parameters = effective.str();
  }


  const std::string row_prefix_header =
    "run_id,timestamp,platform,n_cores,deal_ii_version,backend,"
    "backend_version,dimension,degree,refinement,n_active_cells,n_dofs,"
    "workstream_mode,n_colors,threads,chunk_size";


  struct Output
  {
    Output(const BenchmarkParameters &parameters);

    std::string run_id;
    std::string row_prefix;

    std::ofstream raw;
    std::ofstream summary;
  };


  Output::Output(const BenchmarkParameters &parameters)
  {
    const std::time_t now = std::time(nullptr);

    run_id = parameters.output_prefix + '_' + platform_tag() + "_dealii-" +
             DEAL_II_PACKAGE_VERSION + '_' + backend_name() + "_dim" +
             std::to_string(parameters.dimension) + '_' +
             parameters.mode_selection + '_' +
             formatted_local_time(now, "%Y%m%d-%H%M%S");

    row_prefix = run_id + ',' +
                 formatted_local_time(now, "%Y-%m-%dT%H:%M:%S") + ',' +
                 platform_tag() + ',' +
                 std::to_string(MultithreadInfo::n_cores()) + ',' +
                 DEAL_II_PACKAGE_VERSION + ',' + backend_name() + ',' +
                 backend_version() + ',';

    raw.open(run_id + "_raw.csv");
    summary.open(run_id + "_summary.csv");
    AssertThrow(raw.good() && summary.good(),
                ExcMessage("Cannot write the CSV files for run " + run_id +
                           '.'));

    raw << std::setprecision(15) << row_prefix_header
        << ",phase,repeat,time_sec\n";
    summary << std::setprecision(15) << row_prefix_header
            << ",n_warmup,n_timed,min_time_sec,avg_time_sec,max_time_sec,"
               "stddev_time_sec,reference_matrix_norm\n";
  }


  template <int dim>
  struct AssemblyScratchData
  {
    AssemblyScratchData(const FiniteElement<dim> &fe,
                        const Quadrature<dim>    &quadrature)
      : fe_values(fe,
                  quadrature,
                  update_values | update_gradients | update_JxW_values)
    {}

    AssemblyScratchData(const AssemblyScratchData &scratch_data)
      : fe_values(scratch_data.fe_values.get_fe(),
                  scratch_data.fe_values.get_quadrature(),
                  update_values | update_gradients | update_JxW_values)
    {}

    FEValues<dim> fe_values;
  };


  struct AssemblyCopyData
  {
    AssemblyCopyData(const unsigned int dofs_per_cell)
      : cell_matrix(dofs_per_cell, dofs_per_cell)
      , cell_rhs(dofs_per_cell)
      , local_dof_indices(dofs_per_cell)
    {}

    FullMatrix<double>                   cell_matrix;
    Vector<double>                       cell_rhs;
    std::vector<types::global_dof_index> local_dof_indices;
  };


  struct Configuration
  {
    WorkStreamMode mode;
    unsigned int   n_threads;
    unsigned int   chunk_size;
  };


  template <int dim>
  class WorkStreamBenchmark
  {
  public:
    WorkStreamBenchmark(const BenchmarkParameters &parameters,
                        const unsigned int         degree,
                        const unsigned int         refinement);

    void
    run_sweep(Output &output);

  private:
    using Iterator = typename DoFHandler<dim>::active_cell_iterator;

    void
    setup_system();

    void
    build_graph_coloring();

    void
    local_assemble_system(const Iterator           &cell,
                          AssemblyScratchData<dim> &scratch_data,
                          AssemblyCopyData         &copy_data) const;

    void
    copy_local_to_global(const AssemblyCopyData &copy_data);

    void
    compute_sequential_reference();

    double
    run_workstream_once(const Configuration &configuration);

    std::vector<double>
    measure_configuration(Output &output, const Configuration &configuration);

    double
    relative_error_against_reference() const;

    void
    write_row_prefix(std::ostream        &out,
                     const Output        &output,
                     const Configuration &configuration) const;

    void
    report_configuration(Output                    &output,
                         const Configuration       &configuration,
                         const std::vector<double> &times,
                         const double               serial_mean) const;

    const BenchmarkParameters &parameters;

    const unsigned int degree;
    const unsigned int refinement;

    Triangulation<dim> triangulation;
    FE_Q<dim>          fe;
    QGauss<dim>        quadrature;
    DoFHandler<dim>    dof_handler;

    const AssemblyScratchData<dim> sample_scratch_data;
    const AssemblyCopyData         sample_copy_data;

    SparsityPattern      sparsity_pattern;
    SparseMatrix<double> system_matrix;
    Vector<double>       system_rhs;

    std::vector<std::vector<Iterator>> colored_cell_iterators;
    double                             coloring_wall_time = 0.0;

    double reference_matrix_norm = 0.0;
    double reference_rhs_norm    = 0.0;
  };


  template <int dim>
  WorkStreamBenchmark<dim>::WorkStreamBenchmark(
    const BenchmarkParameters &parameters,
    const unsigned int         degree,
    const unsigned int         refinement)
    : parameters(parameters)
    , degree(degree)
    , refinement(refinement)
    , fe(degree)
    , quadrature(degree + 1)
    , dof_handler(triangulation)
    , sample_scratch_data(fe, quadrature)
    , sample_copy_data(fe.dofs_per_cell)
  {}


  template <int dim>
  void
  WorkStreamBenchmark<dim>::setup_system()
  {
    GridGenerator::hyper_cube(triangulation, -1.0, 1.0);
    triangulation.refine_global(refinement);

    dof_handler.distribute_dofs(fe);

    DynamicSparsityPattern dsp(dof_handler.n_dofs());
    DoFTools::make_sparsity_pattern(dof_handler, dsp);
    sparsity_pattern.copy_from(dsp);

    system_matrix.reinit(sparsity_pattern);
    system_rhs.reinit(dof_handler.n_dofs());
  }


  template <int dim>
  void
  WorkStreamBenchmark<dim>::build_graph_coloring()
  {
    const auto start = std::chrono::steady_clock::now();

    colored_cell_iterators = GraphColoring::make_graph_coloring(
      dof_handler.begin_active(),
      dof_handler.end(),
      [this](const Iterator &cell) {
        std::vector<types::global_dof_index> local_dof_indices(
          fe.dofs_per_cell);
        cell->get_dof_indices(local_dof_indices);
        return local_dof_indices;
      });

    coloring_wall_time =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
        .count();
  }


  template <int dim>
  void
  WorkStreamBenchmark<dim>::local_assemble_system(
    const Iterator           &cell,
    AssemblyScratchData<dim> &scratch_data,
    AssemblyCopyData         &copy_data) const
  {
    scratch_data.fe_values.reinit(cell);

    const unsigned int dofs_per_cell = fe.dofs_per_cell;
    const unsigned int n_q_points    = quadrature.size();

    cell->get_dof_indices(copy_data.local_dof_indices);

    copy_data.cell_matrix = 0.0;
    copy_data.cell_rhs    = 0.0;

    for (unsigned int q_point = 0; q_point < n_q_points; ++q_point)
      for (unsigned int i = 0; i < dofs_per_cell; ++i)
        {
          for (unsigned int j = 0; j < dofs_per_cell; ++j)
            copy_data.cell_matrix(i, j) +=
              (scratch_data.fe_values.shape_grad(i, q_point) *
               scratch_data.fe_values.shape_grad(j, q_point) *
               scratch_data.fe_values.JxW(q_point));

          copy_data.cell_rhs(i) +=
            (scratch_data.fe_values.shape_value(i, q_point) *
             scratch_data.fe_values.JxW(q_point));
        }
  }


  template <int dim>
  void
  WorkStreamBenchmark<dim>::copy_local_to_global(
    const AssemblyCopyData &copy_data)
  {
    system_matrix.add(copy_data.local_dof_indices, copy_data.cell_matrix);
    system_rhs.add(copy_data.local_dof_indices, copy_data.cell_rhs);
  }


  template <int dim>
  void
  WorkStreamBenchmark<dim>::compute_sequential_reference()
  {
    system_matrix = 0.0;
    system_rhs    = 0.0;

    AssemblyScratchData<dim> scratch_data(sample_scratch_data);
    AssemblyCopyData         copy_data(sample_copy_data);

    for (const auto &cell : dof_handler.active_cell_iterators())
      {
        local_assemble_system(cell, scratch_data, copy_data);
        copy_local_to_global(copy_data);
      }

    reference_matrix_norm = system_matrix.frobenius_norm();
    reference_rhs_norm    = system_rhs.l2_norm();
  }


  template <int dim>
  double
  WorkStreamBenchmark<dim>::run_workstream_once(
    const Configuration &configuration)
  {
    system_matrix = 0.0;
    system_rhs    = 0.0;

    const auto worker = [this](const Iterator           &cell,
                               AssemblyScratchData<dim> &scratch_data,
                               AssemblyCopyData         &copy_data) {
      local_assemble_system(cell, scratch_data, copy_data);
    };

    const auto copier = [this](const AssemblyCopyData &copy_data) {
      copy_local_to_global(copy_data);
    };

    // deal.II's default, only the non-colored TBB path actually reads it.
    const unsigned int queue_length = 2 * MultithreadInfo::n_threads();

    // Special remark:
    // 1-thread baseline rows carry chunk_size 0.
    // sequential::run() ignores chunk_size, but WorkStream::run() asserts
    // it's positive.
    const unsigned int chunk_size =
      (configuration.chunk_size > 0 ? configuration.chunk_size : 8);

    const auto start = std::chrono::steady_clock::now();

    if (configuration.mode == WorkStreamMode::colored)
      WorkStream::run(colored_cell_iterators,
                      worker,
                      copier,
                      sample_scratch_data,
                      sample_copy_data,
                      queue_length,
                      chunk_size);
    else
      WorkStream::run(dof_handler.begin_active(),
                      dof_handler.end(),
                      worker,
                      copier,
                      sample_scratch_data,
                      sample_copy_data,
                      queue_length,
                      chunk_size);

    return std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                         start)
      .count();
  }


  template <int dim>
  std::vector<double>
  WorkStreamBenchmark<dim>::measure_configuration(
    Output              &output,
    const Configuration &configuration)
  {
    MultithreadInfo::set_thread_limit(configuration.n_threads);
    AssertThrow(MultithreadInfo::n_threads() == configuration.n_threads,
                ExcMessage("DEAL_II_NUM_THREADS caps the thread limit."));

    if (parameters.cooldown_seconds > 0.0)
      std::this_thread::sleep_for(
        std::chrono::duration<double>(parameters.cooldown_seconds));

    for (unsigned int repeat = 1; repeat <= parameters.n_warmup_repetitions;
         ++repeat)
      {
        const double time_sec = run_workstream_once(configuration);

        write_row_prefix(output.raw, output, configuration);
        output.raw << ",warmup," << repeat << ',' << time_sec << '\n';
      }

    std::vector<double> times;
    times.reserve(parameters.n_timed_repetitions);

    for (unsigned int repeat = 1; repeat <= parameters.n_timed_repetitions;
         ++repeat)
      {
        times.push_back(run_workstream_once(configuration));

        write_row_prefix(output.raw, output, configuration);
        output.raw << ",timed," << repeat << ',' << times.back() << '\n';
      }

    constexpr double norm_relative_tolerance = 1e-10;
    AssertThrow(relative_error_against_reference() < norm_relative_tolerance,
                ExcMessage("Assembly deviates from the sequential reference."));

    return times;
  }


  template <int dim>
  double
  WorkStreamBenchmark<dim>::relative_error_against_reference() const
  {
    return std::max(std::abs(system_matrix.frobenius_norm() -
                             reference_matrix_norm) /
                      reference_matrix_norm,
                    std::abs(system_rhs.l2_norm() - reference_rhs_norm) /
                      reference_rhs_norm);
  }


  template <int dim>
  void
  WorkStreamBenchmark<dim>::write_row_prefix(
    std::ostream        &out,
    const Output        &output,
    const Configuration &configuration) const
  {
    out << output.row_prefix << dim << ',' << degree << ',' << refinement << ','
        << triangulation.n_active_cells() << ',' << dof_handler.n_dofs() << ','
        << to_string(configuration.mode) << ','
        << (configuration.mode == WorkStreamMode::colored ?
              colored_cell_iterators.size() :
              0)
        << ',' << configuration.n_threads << ',' << configuration.chunk_size;
  }


  template <int dim>
  void
  WorkStreamBenchmark<dim>::report_configuration(
    Output                    &output,
    const Configuration       &configuration,
    const std::vector<double> &times,
    const double               serial_mean) const
  {
    const double minimum = *std::min_element(times.begin(), times.end());
    const double maximum = *std::max_element(times.begin(), times.end());
    const double mean    = average(times);

    double variance = 0.0;
    for (const double time : times)
      variance += (time - mean) * (time - mean);
    const double deviation =
      (times.size() > 1 ? std::sqrt(variance / (times.size() - 1)) : 0.0);

    std::cout << "  " << std::left << std::setw(12)
              << to_string(configuration.mode) << std::right << std::setw(8)
              << configuration.n_threads << std::setw(7)
              << configuration.chunk_size << std::fixed << std::setprecision(6)
              << std::setw(14) << minimum << std::setw(14) << mean
              << std::setw(14) << maximum << std::setw(13) << deviation
              << std::setprecision(2) << std::setw(10) << serial_mean / mean
              << '\n';

    write_row_prefix(output.summary, output, configuration);
    output.summary << ',' << parameters.n_warmup_repetitions << ','
                   << times.size() << ',' << minimum << ',' << mean << ','
                   << maximum << ',' << deviation << ','
                   << reference_matrix_norm << '\n';

    output.raw.flush();
    output.summary.flush();
  }


  template <int dim>
  void
  WorkStreamBenchmark<dim>::run_sweep(Output &output)
  {
    setup_system();

    if (parameters.mode_selection != "non-colored")
      build_graph_coloring();

    compute_sequential_reference();

    std::cout << "\nProblem  dim=" << dim << "  Q" << degree
              << "  refinement=" << refinement
              << "  cells=" << triangulation.n_active_cells()
              << "  dofs=" << dof_handler.n_dofs()
              << "  reference matrix norm=" << std::scientific
              << std::setprecision(6) << reference_matrix_norm << '\n';

    if (!colored_cell_iterators.empty())
      {
        std::cout << "  " << colored_cell_iterators.size() << " colors in "
                  << std::fixed << std::setprecision(3) << coloring_wall_time
                  << " s, cells per color:";
        for (const auto &color : colored_cell_iterators)
          std::cout << ' ' << color.size();
        std::cout << '\n';
      }

    std::cout << "  " << std::left << std::setw(12) << "mode" << std::right
              << std::setw(8) << "threads" << std::setw(7) << "chunk"
              << std::setw(14) << "min [s]" << std::setw(14) << "avg [s]"
              << std::setw(14) << "max [s]" << std::setw(13) << "stddev [s]"
              << std::setw(10) << "speedup" << '\n';

    for (const WorkStreamMode mode : parameters.modes)
      {
        const Configuration baseline{mode, 1, 0};

        const std::vector<double> serial_times =
          measure_configuration(output, baseline);
        const double serial_mean = average(serial_times);

        report_configuration(output, baseline, serial_times, serial_mean);

        for (const unsigned int n_threads : parameters.thread_counts)
          {
            if (n_threads == 1) // the baseline above
              continue;

            for (const unsigned int chunk_size : parameters.chunk_sizes)
              {
                const Configuration configuration{mode, n_threads, chunk_size};

                const std::vector<double> times =
                  measure_configuration(output, configuration);

                report_configuration(output, configuration, times, serial_mean);
              }
          }
      }
  }


  template <int dim>
  void
  run_benchmark(const BenchmarkParameters &parameters)
  {
    const auto start = std::chrono::steady_clock::now();

    Output output(parameters);

    const auto describe = [](const std::string &label, const auto &value) {
      std::cout << "  " << std::left << std::setw(21) << label << value << '\n';
    };

    const char *const num_threads = std::getenv("DEAL_II_NUM_THREADS");

    std::cout << "WorkStream release benchmark\n";
    describe("run id:", output.run_id);
    describe("platform:",
             platform_tag() + ", " +
               std::to_string(MultithreadInfo::n_cores()) + " cores");
    describe("deal.II:",
             std::string(DEAL_II_PACKAGE_VERSION) + " (" +
               DEAL_II_GIT_REVISION + ")");
    describe("WorkStream backend:", backend_name() + ' ' + backend_version());
    describe("compiler:", compiler_description());
    describe("DEAL_II_NUM_THREADS:",
             num_threads == nullptr ? std::string("(unset)") :
                                      std::string(num_threads));
    describe("CSV files:", output.run_id + "_{raw,summary}.csv");

#ifndef NDEBUG
    std::cout << "  WARNING: built without NDEBUG, the timings are "
                 "meaningless.\n";
#endif

    std::cout << "\nEffective parameters:\n"
              << parameters.effective_parameters;

    for (const unsigned int refinement : parameters.global_refinements)
      for (const unsigned int degree : parameters.fe_degrees)
        {
          WorkStreamBenchmark<dim> benchmark(parameters, degree, refinement);
          benchmark.run_sweep(output);
        }

    std::cout << "\nFinished in " << std::fixed << std::setprecision(1)
              << std::chrono::duration<double>(
                   std::chrono::steady_clock::now() - start)
                   .count()
              << " s.\n";
  }
} // namespace WorkStreamReleaseBenchmark


int
main(int argc, char *argv[])
{
  using namespace dealii;
  using namespace WorkStreamReleaseBenchmark;

  if (argc != 2)
    {
      std::cerr << "Usage: " << argv[0] << " <input.prm>\n";
      return 1;
    }

  try
    {
      BenchmarkParameters parameters;
      parameters.parse_input_file(argv[1]);

      if (parameters.dimension == 2)
        run_benchmark<2>(parameters);
      else
        run_benchmark<3>(parameters);
    }
  catch (std::exception &exc)
    {
      std::cerr << std::endl
                << std::endl
                << "----------------------------------------------------"
                << std::endl;
      std::cerr << "Exception on processing: " << std::endl
                << exc.what() << std::endl
                << "Aborting!" << std::endl
                << "----------------------------------------------------"
                << std::endl;
      return 1;
    }
  catch (...)
    {
      std::cerr << std::endl
                << std::endl
                << "----------------------------------------------------"
                << std::endl;
      std::cerr << "Unknown exception!" << std::endl
                << "Aborting!" << std::endl
                << "----------------------------------------------------"
                << std::endl;
      return 1;
    }

  return 0;
}
