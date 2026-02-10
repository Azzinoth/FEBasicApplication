# ============================================================================
# UpdateFEBasicApplicationVersion.cmake
# 
# Pre-build script that generates version information for FEBasicApplication.
# Runs every build to capture: git commit counts, branch, hash,
# dirty status and timestamp. Outputs are written to FEBasicApplicationVersion.h via configure_file.
#
# Produces version strings like:
#   Release (on master):  "1.0.0 build 231"
#   Dev branch:           "1.0.0 build 231+52 (dev, ed4c7ce)"
#   Uncommitted changes:  "1.0.0 build 231+52 (dev, ed4c7ce-dirty)"
# ============================================================================

set(FEBASICAPPLICATION_VERSION_MAJOR 0)
set(FEBASICAPPLICATION_VERSION_MINOR 4)
set(FEBASICAPPLICATION_VERSION_PATCH 4)

find_package(Git QUIET)
if(GIT_FOUND)
    # --- Commit counts ---
    # Count total commits on master, this is the stable build number.
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list --count origin/master
        WORKING_DIRECTORY ${FEBASICAPPLICATION_FOLDER}
        OUTPUT_VARIABLE FEBASICAPPLICATION_MASTER_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Count commits ahead of master on current branch.
    # Will be 0 when building on master itself.
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list --count origin/master..HEAD
        WORKING_DIRECTORY ${FEBASICAPPLICATION_FOLDER}
        OUTPUT_VARIABLE FEBASICAPPLICATION_BRANCH_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # --- Commit hash ---
    # Short hash for exact commit identification.
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${FEBASICAPPLICATION_FOLDER}
        OUTPUT_VARIABLE FEBASICAPPLICATION_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # --- Branch name ---
    # Get current branch name. Returns "HEAD" when in detached HEAD state.
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${FEBASICAPPLICATION_FOLDER}
        OUTPUT_VARIABLE FEBASICAPPLICATION_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # --- Dirty status ---
    # Check for uncommitted changes (staged or unstaged).
    # git diff --quiet returns exit code 1 if there are changes.
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff --quiet HEAD
        WORKING_DIRECTORY ${FEBASICAPPLICATION_FOLDER}
        RESULT_VARIABLE GIT_DIRTY_RC
    )
    if(GIT_DIRTY_RC EQUAL 0)
        set(FEBASICAPPLICATION_GIT_DIRTY 0)
    else()
        set(FEBASICAPPLICATION_GIT_DIRTY 1)
    endif()
else()
    # Fallback when git is not available (e.g. source archive without .git)
    set(FEBASICAPPLICATION_MASTER_COMMIT_COUNT 0)
    set(FEBASICAPPLICATION_BRANCH_COMMIT_COUNT 0)
    set(FEBASICAPPLICATION_GIT_HASH "unknown")
    set(FEBASICAPPLICATION_GIT_BRANCH "unknown")
    set(FEBASICAPPLICATION_GIT_DIRTY 0)
endif()

# --- Detached HEAD resolution ---
# Submodules are typically checked out in detached HEAD state.
# Try to resolve the actual branch name from remote tracking branches.
if("${FEBASICAPPLICATION_GIT_BRANCH}" STREQUAL "HEAD")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} branch -r --contains HEAD
        WORKING_DIRECTORY ${FEBASICAPPLICATION_FOLDER}
        OUTPUT_VARIABLE FEBASICAPPLICATION_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    # Take the first matching remote branch and strip "origin/" prefix.
    if(NOT "${FEBASICAPPLICATION_GIT_BRANCH}" STREQUAL "")
        string(REGEX MATCH "[^ \n]+" FEBASICAPPLICATION_GIT_BRANCH "${FEBASICAPPLICATION_GIT_BRANCH}")
        string(REGEX REPLACE "^origin/" "" FEBASICAPPLICATION_GIT_BRANCH "${FEBASICAPPLICATION_GIT_BRANCH}")
    else()
        set(FEBASICAPPLICATION_GIT_BRANCH "detached")
    endif()
endif()

# --- Build timestamp ---
# Timestamp in YYYYMMDDHHmmSS format.
string(TIMESTAMP FEBASICAPPLICATION_BUILD_TIMESTAMP \"%Y%m%d%H%M%S\")

# --- Generate header ---
configure_file(${FEBASICAPPLICATION_FOLDER}/FEBasicApplicationVersion.h.in ${FEBASICAPPLICATION_FOLDER}/FEBasicApplicationVersion.h @ONLY)