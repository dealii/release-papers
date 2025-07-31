Copy and paste the following markdown list into a deal.II Issue:

## Pre-Release testing, QA and final code changes:

The following is a list of ouststanding QA tests that should be checked before proceeding with a release. In order to avoid duplicated efforts, add your a tag `@name` with your name after `assigned:` and link corresponding pull requests after `pr:`. Finally, tick of the box when the task is done.

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
   ```./contrib/utilities/update-copyright.sh```
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

The following steps have to be done by a single developer (with admin privileges on the [repository](https://github.com/dealii/dealii))

 - [ ] <b>Create and push a release branch</b>
   * Verify that the local `master` branch is up to date and create local branch:
     ```git checkout master && git pull```
     ```git status```
     ```git checkout -b dealii-9.5``` (<b>Note:</b> exactly `dealii-9.5` to remain consistent!)
   * Update `VERSION` file from `9.5.0-pre` to `9.5.0-rc0`:
     ```echo 9.5.0-rc0 > VERSION```
     <b>Note:</b> Please do not use `9.5.0`, or `9.5.0-rc1`, directly because the soname is derived from the current version number. Therefore, 9.5.0 should only be used for the final, tagged and distributed code.
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
     ```echo 9.6.0-pre > VERSION```
     <b>Note:</b> Conservatively only increment the minor version even if the plan is to later release a major update &mdash; it is always possible to convert 9.4 into 10.0 later, but one can't easily back down any more.
   * ```git commit -a -m "Update VERSION"```
   * Make a pull request against main branch (`master`).

## Post-Branching tasks and cleanup:

The following is a list of ouststanding tasks that have to happen on the release branch before we can tag a release (candidate). In order to avoid duplicated efforts, add your a tag `@name` with your name after `assigned:` and link corresponding pull requests after `pr:`. Finally, tick of the box when the task is done.

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


## Create a release:

The following steps have to be done by a single developer (with admin privileges on the [repository](https://github.com/dealii/dealii))

 - [ ] <b>Push the button</b>
   * Update `VERSION` file from `9.5.0-rc0` to `9.5.0-rc1` (or `9.5.0` for the final release):
     ```echo 9.5.0-rc1 > VERSION```
     ```git commit -a -m "update VERSION for release"```
     <b>Note:</b> This commit is done without a pull request! We use the `update VERSION` commits to tag releases and release candidates and therefore should be plain commits on the release branch.
   * Create a signed tag for the commit:
     ```git tag -s -m "deal.II Pre-Release Version 9.5.0-rc1" v9.5.0-rc1```
     or alternatively for a release:
     ```git tag -s -m "deal.II Version 9.5.0" v9.5.0```
     You need a working gnupg key for this. (You should have anyway :-P)
   * Push the changes to github (assuming `origin` points to github):
     ```git push origin dealii-9.5 v9.5.0-rc1```
 - [ ] <b>Create and sign tar archive</b>
   * Github automatically generates a source tarball from the current repository state, see https://github.com/dealii/dealii/releases
   * Download the source tarball from github, verify that its contents is what we expect, e.g., by doing the following in a clean repository:
     ```git checkout v9.5.0-rc1```
     ```tar --strip-components=1 -xvf dealii-9.5.0-rc1.tar.gz```
     ```git status```
     Now, the last git status will only record some deleted files but must not show any modified file contents!
   * Sign it:
     ```gpg --detach-sign --armor dealii-9.5.0-rc1.tar.gz```
 - [ ] <b>Create and sign documentation tar archive</b>
   * Check out the code-gallery at the correct path
   * Install MathJax locally and export the path, for example on Debian/Gentoo:
     ```export MATHJAX_ROOT="/usr/share/mathjax"```
   * Generate and install the documentation from a clean working directory containing the tagged release and configure with:
     ```cmake -DDOCUMENTATION=ON -DDEAL_II_DOXYGEN_USE_MATHJAX=ON -DDEAL_II_DOXYGEN_USE_ONLINE_MATHJAX=OFF \```
     ```      -DDEAL_II_COMPILE_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX=<install>```
     ```make documentation```
     ```make examples```
   * Download images:
     ```<install>/doc/doxygen/deal.II```
     ```<source dir>/contrib/utilities/makeofflinedoc.sh```
   * (In the same directory) copy mathjax and fix includes:
     ```cp -r "$MATHJAX_ROOT" mathjax```
     ```sed -i -e "s#$MATHJAX_ROOT#mathjax#" **/*.html```
   * Grep the documentation for "full path leaks", for example grep for your user name and the git repository location and fix up all remaining locations (in particular deal.tag)
   * Create `dealii-9.5.0-rc1-offline_documentation.tar.gz`:
     ```cd <install>```
     ```tar --numeric-owner --owner=0 --group=0 -cvf dealii-9.5.0-rc1-offline_documentation.tar doc examples```
     ```gzip dealii-9.2.0-offline_documentation.tar```
   * And sign it:
     ```gpg --detach-sign --armor dealii-9.5.0-rc1-offline_documentation.tar.gz```
 - [ ] <b>Create a (pre-)release on github</b>
   * Got to https://github.com/dealii/dealii/tags and select the just uploaded v9.5.0-rc1 tag and click on "add release notes". Use the following information:
     ```Tag: v9.0.0-rc1 existing tag```
     ```Release title: deal.II pre-release version 9.0.0-rc1```
     ```Description: ""```
     <b>Note:</b> For a release use:
     ```Tag: dealii-9.5.0 existing tag```
     ```Release title: deal.II version 9.5.0```
     ```Description: [Short version of release notes as found in announce-9.5]```
     ```A full list of changes can be found at```
     ```https://www.dealii.org/developer/doxygen/deal.II/changes_between_9_4_0_and_9_5_0.html```
   * Attach `dealii-9.5.0-rc1.tar.gz` (yes, once again), `dealii-9.5.0-rc1.tar.gz.asc`, `dealii-9.5.0-rc1-offline_documentation.tar.gz`, `dealii-9.5.0-rc1-offline_documentation.tar.gz.asc`.

## Post (Pre-)release steps:

The following is a list of ouststanding tasks that have to happen after we tagged a release (candidate). In order to avoid duplicated efforts, add your a tag `@name` with your name after `assigned:`. Finally, tick of the box when the task is done.

 - [ ] <b>Generate the documentation on the webserver</b> (assigned:)
 - [ ] <b>Adjust `header.include` on the homepage</b> (assigned:)
   Change links to the documentation of the most recent version; change the link to the changes_after_X_Y_Z.html file.
 - [ ] <b>Adjust `news.html` on the homepage</b> (assigned:)
   * add a new entry for the release using the link to the copied changes.h file from above
   * also rotate the news entry in the short news blurbb on the fron page in `index.html`
 - [ ] <b>Generate MAC bundles and attach to current (pre-)release</b> (assigned:)
 - [ ] <b>Update deal.II package in Spack</b> (assigned:)
 - [ ] <b>Update deal.II package in Candi</b> (assigned:)
 - [ ] <b>Update deal.II package in Gentoo</b> (assigned:)
 - [ ] <b>Update deal.II package in Ubuntu/Debian and generate PPA for Ubuntu LTS</b> (assigned:)
 - [ ] <b>Update deal.II package in Archlinux AUR</b> (assigned:)
 - [ ] <b>Update deal.II Virtualbox Images</b> (assigned:)
 - [ ] <b>Release paper</b>
   * [ ] Copy the directory from the previous release
   * [ ] Strip all non-generic content
   * [ ] Update the list of contributors
   * [ ] Finish all sections
   * [ ] Put the completed manuscript onto the website
   * [ ] Submit to the journal we usually publish it in
   * [ ] Put a link onto the publications page
   * [ ] Once published, put the final link onto the publications page
 - [ ] <b>Release announcements</b> (assigned: &mdash;)
   * [ ] Write it, based on the examples that are in previous release paper directories
   * [ ] Send it to the user mailing list
   * [ ] Send a separate mail with the list of contributors for this release
   * [ ] Post on social media (facebook, twitter)
   * [ ] Send a news item to NADigest; web form breaks longer lines automatically, so keep them long to avoid awkward formatting; https://na-digest.coecis.cornell.edu/submit/
   * [ ] SIAM CS&E digest
 - [ ] <b>Update wikipedia pages</b> (assigned:)
   * https://en.wikipedia.org/wiki/Deal.II
   * https://en.wikipedia.org/wiki/List_of_finite_element_software_packages
