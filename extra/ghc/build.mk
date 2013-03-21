# -----------------------------------------------------------------------------
# A Sample build.mk
#
# Uncomment one of the following BuildFlavour settings to get the desired
# overall build type, and then tweak the options in the relevant section
# below.

# Uncomment one of these to select a build profile below:

# Full build with max optimisation and everything enabled (very slow build)
#BuildFlavour = perf

# As above but build GHC using the LLVM backend
BuildFlavour = perf-llvm

# Fast build with optimised libraries, no profiling (RECOMMENDED):
#BuildFlavour = quick

# Even faster build.  NOT RECOMMENDED: the libraries will be
# completely unoptimised, so any code built with this compiler
# (including stage2) will run very slowly:
#BuildFlavour = quickest

# Profile the stage2 compiler:
#BuildFlavour = prof

# A development build, working on the stage 1 compiler:
#BuildFlavour = devel1

# A development build, working on the stage 2 compiler:
#BuildFlavour = devel2

# An unregisterised, optimised build of ghc, for porting:
#BuildFlavour = unreg

GhcLibWays = v

# -------- 1. A Performance/Distribution build--------------------------------

ifeq "$(BuildFlavour)" "perf"

# perf matches the default settings, repeated here for comparison:

SRC_HC_OPTS     = -O -H64m
GhcStage1HcOpts = -O -fasm
GhcStage2HcOpts = -O2 -fasm
GhcHcOpts       = -Rghc-timing
GhcLibHcOpts    = -O2
GhcLibWays     += p

ifeq "$(PlatformSupportsSharedLibs)" "YES"
GhcLibWays += dyn
endif

endif

# ---------------- Perf build using LLVM -------------------------------------

ifeq "$(BuildFlavour)" "perf-llvm"

SRC_HC_OPTS     = -O -H64m -fllvm
GhcStage1HcOpts = -O -fllvm
GhcStage2HcOpts = -O2 -fllvm
GhcHcOpts       = -Rghc-timing
GhcLibHcOpts    = -O2
GhcLibWays     += p

ifeq "$(PlatformSupportsSharedLibs)" "YES"
GhcLibWays += dyn
endif

endif

# -------- A Fast build ------------------------------------------------------

ifeq "$(BuildFlavour)" "quickest"

SRC_HC_OPTS        = -H64m -O0 -fasm
GhcStage1HcOpts    = -O -fasm
GhcStage2HcOpts    = -O0 -fasm
GhcLibHcOpts       = -O0 -fasm
SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO

endif

# -------- A Fast build with optimised libs ----------------------------------

ifeq "$(BuildFlavour)" "quick"

SRC_HC_OPTS        = -H64m -O0 -fasm
GhcStage1HcOpts    = -O -fasm
GhcStage2HcOpts    = -O0 -fasm
GhcLibHcOpts       = -O -fasm
SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO

endif

# -------- Profile the stage2 compiler ---------------------------------------

ifeq "$(BuildFlavour)" "prof"

SRC_HC_OPTS        = -H64m -O0 -fasm
GhcStage1HcOpts    = -O -fasm
GhcStage2HcOpts    = -O -fasm
GhcLibHcOpts       = -O -fasm

GhcLibWays         += p
GhcProfiled        = YES

SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO

endif


# -------- A Development build (stage 1) -------------------------------------

ifeq "$(BuildFlavour)" "devel1"

SRC_HC_OPTS        = -H64m -O -fasm
GhcLibHcOpts       = -O -dcore-lint
GhcStage1HcOpts    = -Rghc-timing -O0 -DDEBUG
GhcStage2HcOpts    = -Rghc-timing -O -fasm
SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO
LAX_DEPENDENCIES   = YES

endif

# -------- A Development build (stage 2) -------------------------------------

ifeq "$(BuildFlavour)" "devel2"

SRC_HC_OPTS        = -H64m -O -fasm
GhcLibHcOpts       = -O -dcore-lint
GhcStage1HcOpts    = -Rghc-timing -O -fasm
GhcStage2HcOpts    = -Rghc-timing -O0 -DDEBUG
SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO
LAX_DEPENDENCIES   = YES

# After stage 1 and the libraries have been built, you can uncomment this line:

# stage=2

# Then stage 1 will not be touched by the build system, until
# you comment the line again.  This is a useful trick for when you're
# working on stage 2 and want to freeze stage 1 and the libraries for
# a while.

endif

# -------- A Unregisterised build) -------------------------------------------

ifeq "$(BuildFlavour)" "unreg"

# Note that the LLVM backend works in unregisterised mode as well as
# registerised mode. This often makes it a good choice for porting
# GHC.

GhcUnregisterised    = YES
GhcWithNativeCodeGen = NO

SRC_HC_OPTS          = -O -H64m # -fllvm
GhcStage1HcOpts      = -O
GhcStage2HcOpts      = -O2
GhcHcOpts            = -Rghc-timing
GhcLibHcOpts         = -O2
SplitObjs            = NO
HADDOCK_DOCS         = NO
BUILD_DOCBOOK_HTML   = NO
BUILD_DOCBOOK_PS     = NO
BUILD_DOCBOOK_PDF    = NO

endif

# -----------------------------------------------------------------------------
# Other settings that might be useful

# NoFib settings
NoFibWays =
STRIP_CMD = :

