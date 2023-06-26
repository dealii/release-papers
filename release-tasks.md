Copy and paste the following markdown list into a deal.II Issue:

## Pre-Release testing, QA and final code changes:

The following is a list of ouststanding QA tests that should be checked before proceeding with a release. In order to duplicated efforts, add your a tag `@name` with your name after `assigned:` and link corresponding pull requests after `pr:`. finally, tick of the box when the task is done.

 - [ ] <b>Update regression tester</b> (assigned:, pr: &mdash;)
   Check and (if necessary) update testing infrastructure so that at least one configuration runs with the latest versions of external dependencies and compilers.
   Also temporarily remove all test exclusion lists from the regression tester and re-evaluate whether they still have to be excluded.
 - [ ] <b>Outstanding issues and pull-requests</b>
   Check issues and pull-requests for outstanding fixes.
   Update release milestones https://github.com/dealii/dealii/milestone/12
 - [ ] <b>cppcheck</b> (assigned:, pr:)
   Run `cppcheck` across all source files:
   ```cppcheck --force -j4 --enable=all -I./include/ ./source/*/*.cc >cppcheck-results.txt 2>&1```
   and look for anything that should obviously be fixed.
 - [ ] <b>codespell</b> (assigned:, pr:)
   Run `codespell` across all source files:
   ```codespell ./contrib/ ./cmake/ ./doc/ ./examples/ ./include/ ./source/```
   and look for typos and spelling mistakes that should be fixed.
 - [ ] <b>clang-tidy</b> (assigned:, pr:)
   Run clang tidy via:
   ```mkdir build && cd build```
   ```../contrib/utilities/run_clang_tidy.sh ../```
   and check that no warnings are reported.
 - [ ] <b>doxygen reflinks</b> (assigned:, pr:)
   Check doxygen commands and reflinks:
   ```cd include```
   ```find . -iname "*h" -exec ../contrib/utilities/wrapcomments.py {} \; > /dev/null```
   Also check:
   ```grep -l '@ref[^"]*"[^"]*$' **/*.h```
   ```grep -l '@ref$' **/*.h```
   <b>NOTE:</b> Do not use wrapcomments.py to actually reflow comments - the script causes too much churn with little value.
 - [ ] <b>doxygen</b> (assigned:, pr:)
   Check that doxygen produces no errors when generating the documentation. Also check that the tutorial dot graph gets generated.
 - [ ] <b>Copyright years</b> (assigned:, pr:)
   Run the copyright script:
   ```./contrib/utilities/update-copyright```
   and make a pull request with a commit title "Update copyright years".
 - [ ] <b>Examples</b> (assigned:, pr:)
   Configure, build and run all examples by hand:
   ```cd examples```
   ```for i in step-* ; do (cd $i ; cmake -DDEAL_II_DIR=../.. . ; make ; make run) ; done```
   This step will require looking at the output and checking whether
   anything untoward is happening.
 - [ ] <b>Code Gallery</b> (assigned:, pr: &mdash;)
   Do the same for the code gallery examples: They should at the very least compile. If possible, also run them to make sure they don't trip up any exceptions.
 - [ ] <b>Update `.gitattributes`</b> (assigned:, pr:)
       Check that `.gitattributes` is up to date by using `git archive` and checking that the contents is meaningful. In particular, do `quicktests` still run?
 - [ ] <b>Update authors</b> (assigned:, pr: &mdash;)
   Update the authors list on the homepage authors.html by cross-referencing `doc/news/changes.h` and the git commit history:
   ```egrep '[0-9]+/[0-9]+/[0-9]+' doc/news/changes/*/* | perl -p -e 's/^.*\(//g; s#, *[0-9/]+\)##g; s/, */\n/g;' | sort | uniq```
   Compare with the git commit history between the last release and current master:
   ```git log --date=short --format="%an %aE" v9.4.0..HEAD | grep -v 'dependabot\[bot\]' | sort | uniq```
   The list of contributors to the current release should also be copied to the end of the release paper.
 - [ ] <b>Changes file</b> (assigned:, pr:)
   This step should happen immediately before branching off so that the changes file does not need to get updated again.
   * Create a new changes files by running the following commands:
     ```mkdir build-doc && cd build-doc```
     ```cmake -DDOCUMENTATION=ON ../```
     ```make documentation```
     ```cp doc/news/changes.h ../doc/news/9.4.0-vs-9.5.0.h```
     ```cd ../```
   * Manually check `doc/news/9.4.0-vs-9.5.0.h` for dead links, duplicated entries, obsolete entries (e.g., commit 1 adds feature A, commit 2 fixes feature A), etc. Update `@page` title and copyright year.
   * Then,
     ```git checkout -b update-changes-file```
     ```git add  doc/news/9.4.0-vs-9.5.0.h```
     ```git commit -m "Update the changelog file for the release."```
     ```git rm doc/news/changes/{major,minor,incompatibilities}/*
     ```git commit -m "Remove now obsolete files."```
     ```touch doc/news/changes/{major,minor,incompatibilities}/20230623dummy```
     ```git add doc/news/changes/{major,minor,incompatibilities}/20230623dummy```
     ```git commit -m "Add dummy files."```
   * Adjust `doc/users/doxygen.html` to link to the to-be-created TAG file.
   * Make a pull request


## Branching off the release branch:

The following steps have to be done by a single developer (with admin privileges on the https://github.com/dealii/dealii repository)

 - [ ] <b>Create and push a release branch</b>
   * Verify that the local `master` branch is up to date and create local branch:
     ```git checkout master && git pull```
     ```git status```
     ```git checkout -b dealii-9.5``` (<b>Note:</b> exactly `dealii-9.5` to remain consistent!)
   * Update `VERSION` file from `9.5.0-pre` to `9.5.0-rc0`:
     ```echo 9.5.0-rc0 > VERSION```
     <b>Note:</b> Please do not use `9.5.0`, or `9.5.0-rc1`, directly because the soname is derived from the current version number. Therefore, 9.5.0 chould only be used for the final, tagged and distributed code.
     ```git commit -a -m "update VERSION"```
     <b>Note:</b> This commit is done without a pull request! We use the `update VERSION` commits to tag releases and release candidates and therefore should be plain commits on the release branch.
   * Push the branch to github (assuming `origin` points to github):
     ```git push origin dealii-9.5`
 - [ ] <b>Github: Update branch properties</b>
       Check branch properties on github! This should be set automatically but nevertheless make sure that the following options are set: push protection + mandatory test + mandatory number of reviews, best between 2 and 3.
 - [ ] <b>Main branch: Update version</b>
     ```git checkout master```
     ```git checkout -b update_version```
   * Update the `VERSION` file from `9.5.0-pre` to `9.6.0-pre`.
     ```echo 9.6.0-rc0 > VERSION```
     <b>Note:</b> Conservatively only increment the minor version even if the plan is to later release a major update &mdash; it is always possible to convert 9.4 into 10.0 later, but one can't easily back down any more.
   * ```git commit -a -m "Update VERSION"```
   * Make a pull request against main branch (`master`).

## Post-Branching tasks and cleanup:

The following is a list of ouststanding tasks that have to happen on the release branch before we can tag a release (candidate). In order to duplicated efforts, add your a tag `@name` with your name after `assigned:` and link corresponding pull requests after `pr:`. finally, tick of the box when the task is done.

 - [ ] <b>Update `AUTHORS.md` and `LICENSE.md`</b> (assigned:, pr:)
   * Create an `AUTHORS.md` file in the top-level directory of the branch that contains a text-only copy of the authors.html file from the website. You can start from lynx -dump -nolist https://www.dealii.org/authors.html >AUTHORS and format it like this: https://github.com/dealii/dealii/blob/dealii-9.0/AUTHORS
   * Update `LICENSE.md` to point to the AUTHORS.md file:
     ```[...] refers to the people listed in the file AUTHORS.md.```
   * ```git add AUTHORS.md LICENSE.md```
     ```git commit -a -m "update AUTHORS.md and LICENSE.md"```
   * Make a pull request against release branch (`dealii-9.5`).
 - [ ] <b>Remove unfinished tutorial programs.</b> (assigned:, pr:)
   * Delete directories on the release branch:
     ```cd examples```
     ```git rm -r step-... ...```
   * These steps should also not be listed on the branch in `doc/doxygen/tutorial/tutorial.h.in`
   * ```git commit -a -m "remove unfinished example steps"```
   * Make a pull request against the release branch (`dealii-9.5`).
 - [ ] <b>Update deprecations</b> (assigned:, pr:)
   On the main branch, replace all occurrences of `DEAL_II_DEPRECATED_EARLY` by `DEAL_II_DEPRECATED` in the library.
   <b>Note:</b> This is a time critical task so that we don't lose track what declarations have been deprecated (early) before or after this release.
 - [ ] <b>Update version dependencies</b> (assigned:, pr:)
   * On mainline, do something like this to require the current dev version for the tutorials and in the documentation:
     ```perl -pi -e 's/deal.II 9.5.0/deal.II 9.6.0/g;' examples/*/CMakeLists.txt```
   * Also check `doc/users/*` and `tests/*`:
     ```grep "FIND_PACKAGE(deal.II" -r doc/ tests/```
   * ```git commit -a -m "Require the current version of deal.II."```
   * Make a pull request against main branch (`master`).
 - [ ] <b>Documentation check</b> (assigned:, pr:)
   Ensure once more that the documentation is generated cleanly:
   * Ensure that the code gallery is in place:
     ```git clone https://github.com/dealii/code-gallery.git```
   * Generate documentation
     ```mkdir build && cd build```
     ```cmake -DDOCUMENTATION=ON -DDEAL_II_DOXYGEN_USE_MATHJAX=ON -DCMAKE_INSTALL_PREFIX=<install>```
     ```make documentation```
     ```cd <install>/doc```
     ```for i in `find . | egrep '\.html$'` ; do perl <source>/doc/doxygen/scripts/validate-xrefs.pl $i ; done```
