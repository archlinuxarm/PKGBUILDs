# -----------------------------------------------------------------------------
# A Sample build.mk
#
# Uncomment one of the following BuildFlavour settings to get the desired
# overall build type, and then tweak the options in the relevant section
# below.

# -------- Build profiles -----------------------------------------------------
# Uncomment one of these to select a build profile below:

# Full build with max optimisation and everything enabled (very slow build)
#BuildFlavour = perf

# As above but build GHC using the LLVM backend
BuildFlavour = perf-llvm

# Perf build configured for a cross-compiler
#BuildFlavour = perf-cross

# Fast build with optimised libraries, no profiling (RECOMMENDED):
#BuildFlavour = quick

# Fast build with optimised libraries, no profiling, with LLVM:
#BuildFlavour = quick-llvm

# Fast build configured for a cross compiler
#BuildFlavour  = quick-cross

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

# A build with max optimisation that still builds the stage2 compiler
# quickly. Compiled code will be the same as with "perf". Programs
# will compile more slowly.
#BuildFlavour = bench

# As above but build GHC using the LLVM backend
#BuildFlavour = bench-llvm

# Bench build configured for a cross-compiler
#BuildFlavour = bench-cross

# -------- Miscellaneous variables --------------------------------------------

# Set to V = 0 to get prettier build output.
# Please use V = 1 when reporting GHC bugs.
V = 1

# After stage 1 and the libraries have been built, you can uncomment this line:

#stage=2

# Then stage 1 will not be touched by the build system, until
# you comment the line again.  This is a useful trick for when you're
# working on stage 2 and want to freeze stage 1 and the libraries for
# a while.

# Uncomment the following line to enable building DPH
#BUILD_DPH=YES

GhcLibWays = $(if $(filter $(DYNAMIC_GHC_PROGRAMS),YES),v dyn,v)

# Only use -fasm by default on platforms that support it.
GhcFAsm = $(if $(filter $(GhcWithNativeCodeGen),YES),-fasm,)

# ----------- A Performance/Distribution build --------------------------------

ifeq "$(BuildFlavour)" "perf"

# perf matches the default settings, repeated here for comparison:

SRC_HC_OPTS     = -O -H64m
GhcStage1HcOpts = -O $(GhcFAsm)
GhcStage2HcOpts = -O2 $(GhcFAsm)
GhcHcOpts       = -Rghc-timing
GhcLibHcOpts    = -O2
GhcLibWays     += p

ifeq "$(PlatformSupportsSharedLibs)" "YES"
GhcLibWays += dyn
endif

endif

# ---------------- Perf build using LLVM --------------------------------------

ifeq "$(BuildFlavour)" "perf-llvm"

SRC_HC_OPTS     = -O -H64m -fllvm -optc-mlong-calls
GhcStage1HcOpts = -O -fllvm
GhcStage2HcOpts = -O2 -fllvm
GhcHcOpts       = -Rghc-timing
GhcLibHcOpts    = -O2
GhcLibWays     += p

endif

# -------- A Fast build -------------------------------------------------------

ifeq "$(BuildFlavour)" "quickest"

SRC_HC_OPTS        = -H64m -O0 $(GhcFAsm)
GhcStage1HcOpts    = -O $(GhcFAsm)
GhcStage2HcOpts    = -O0 $(GhcFAsm)
GhcLibHcOpts       = -O0 $(GhcFAsm)
SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO

endif

# -------- A Fast build with optimised libs -----------------------------------

ifeq "$(BuildFlavour)" "quick"

SRC_HC_OPTS        = -H64m -O0 $(GhcFAsm)
GhcStage1HcOpts    = -O $(GhcFAsm)
GhcStage2HcOpts    = -O0 $(GhcFAsm)
GhcLibHcOpts       = -O $(GhcFAsm)
SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO

endif

# -------- A Fast build with optimised libs using LLVM ------------------------

ifeq "$(BuildFlavour)" "quick-llvm"

SRC_HC_OPTS        = -H64m -O0 -fllvm
GhcStage1HcOpts    = -O -fllvm
GhcStage2HcOpts    = -O0 -fllvm
GhcLibHcOpts       = -O -fllvm
SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO

endif

# -------- A Fast build configured for cross-compilation ----------------------

ifeq "$(BuildFlavour)" "quick-cross"

SRC_HC_OPTS        = -H64m -O0
GhcStage1HcOpts    = -O
GhcStage2HcOpts    = -O0 -fllvm
GhcLibHcOpts       = -O -fllvm
SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO
INTEGER_LIBRARY    = integer-simple
Stage1Only         = YES

DYNAMIC_BY_DEFAULT   = NO
DYNAMIC_GHC_PROGRAMS = NO

endif

# -------- Profile the stage2 compiler ----------------------------------------

ifeq "$(BuildFlavour)" "prof"

SRC_HC_OPTS        = -H64m -O0 $(GhcFAsm)
GhcStage1HcOpts    = -O $(GhcFAsm)
GhcStage2HcOpts    = -O $(GhcFAsm)
GhcLibHcOpts       = -O $(GhcFAsm)

GhcLibWays         += p
GhcProfiled        = YES

SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO

endif

# -------- A Development build (stage 1) --------------------------------------

ifeq "$(BuildFlavour)" "devel1"

SRC_HC_OPTS        = -H64m -O $(GhcFAsm)
GhcLibHcOpts       = -O -dcore-lint
GhcStage1HcOpts    = -Rghc-timing -O0 -DDEBUG
GhcStage2HcOpts    = -Rghc-timing -O $(GhcFAsm)
SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO
LAX_DEPENDENCIES   = YES

endif

# -------- A Development build (stage 2) --------------------------------------

ifeq "$(BuildFlavour)" "devel2"

SRC_HC_OPTS        = -H64m -O $(GhcFAsm)
GhcLibHcOpts       = -O -dcore-lint
GhcStage1HcOpts    = -Rghc-timing -O $(GhcFAsm)
GhcStage2HcOpts    = -Rghc-timing -O0 -DDEBUG
SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO
LAX_DEPENDENCIES   = YES

endif

# -------- A bench build with optimised libs -----------------------------------

ifeq "$(BuildFlavour)" "bench"

SRC_HC_OPTS        = -O -H64m
GhcStage1HcOpts    = -O $(GhcFAsm)
GhcStage2HcOpts    = -O0 $(GhcFAsm)
GhcLibHcOpts       = -O2 $(GhcFAsm)
SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO

endif

# ---------------- Bench build using LLVM --------------------------------------

ifeq "$(BuildFlavour)" "bench-llvm"

SRC_HC_OPTS        = -O -H64m
GhcStage1HcOpts    = -O -fllvm
GhcStage2HcOpts    = -O0 -fllvm
GhcLibHcOpts       = -O2 -fllvm
SplitObjs          = NO
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO

endif

# ------- A Bench build configured for cross-compilation ----------------------

ifeq "$(BuildFlavour)" "bench-cross"

SRC_HC_OPTS        = -O -H64m
GhcStage1HcOpts    = -O $(GhcFAsm)
GhcStage2HcOpts    = -O0 $(GhcFAsm)
GhcLibHcOpts       = -O2 $(GhcFAsm)
SplitObjs          = NO
INTEGER_LIBRARY    = integer-simple
Stage1Only         = YES
HADDOCK_DOCS       = NO
BUILD_DOCBOOK_HTML = NO
BUILD_DOCBOOK_PS   = NO
BUILD_DOCBOOK_PDF  = NO

DYNAMIC_BY_DEFAULT   = NO
DYNAMIC_GHC_PROGRAMS = NO

endif

# -----------------------------------------------------------------------------
# Other settings that might be useful

# NoFib settings
NoFibWays =
STRIP_CMD = :

