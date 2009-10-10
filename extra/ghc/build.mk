# -----------------------------------------------------------------------------
# A Sample build.mk
#
# Uncomment one of the following BuildFlavour settings to get the desired
# overall build type, and then tweak the options in the relevant section
# below.

# Uncomment one of these to select a build profile below:

# Full build with max optimisation (slow build)
BuildFlavour = perf

# Fastest build (libs unoptimised):
#BuildFlavour = quickest

# Fast build with optimised libraries:
#BuildFlavour = quick

# A development build, working on the stage 1 compiler:
#BuildFlavour = devel1

# A development build, working on the stage 2 compiler:
#BuildFlavour = devel2

# Which warnings we like to use
MyWarningOpts   = -W -fno-warn-unused-matches -fwarn-unused-imports

# -------- 1. A Performance/Distribution build--------------------------------

ifeq "$(BuildFlavour)" "perf"

# perf matches the default settings, repeated here for comparison:

SRC_HC_OPTS     = -O -H64m
GhcStage1HcOpts = -O -fasm
GhcStage2HcOpts = -O2 -fasm
GhcHcOpts       = -Rghc-timing
GhcLibHcOpts    = -O2 -XGenerics
GhcLibWays      = p

endif

# -------- A Fast build ------------------------------------------------------

ifeq "$(BuildFlavour)" "quickest"

SRC_HC_OPTS     = -H64m -O0 -fasm
GhcStage1HcOpts = -O -fasm
GhcStage2HcOpts = -O0 -fasm
GhcLibHcOpts    = -O0 -fasm
GhcLibWays      =
SplitObjs       = NO

endif

# -------- A Fast build with optimised libs ----------------------------------

ifeq "$(BuildFlavour)" "quick"

SRC_HC_OPTS     = -H64m -O0 -fasm
GhcStage1HcOpts = -O -fasm
GhcStage2HcOpts = -O0 -fasm
GhcLibHcOpts    = -O -fasm
GhcLibWays      =
SplitObjs       = NO

endif

# -------- A Development build (stage 1) -------------------------------------

ifeq "$(BuildFlavour)" "devel1"

SRC_HC_OPTS     = -H64m -O -fasm $(MyWarningOpts)
GhcLibHcOpts    = -O -dcore-lint $(MyWarningOpts)
GhcLibWays      =
GhcStage1HcOpts = -Rghc-timing -O0 -DDEBUG
GhcStage2HcOpts = -Rghc-timing -O -fasm
SplitObjs = NO

endif

# -------- A Development build (stage 2) -------------------------------------

ifeq "$(BuildFlavour)" "devel2"

SRC_HC_OPTS     = -H64m -O -fasm $(MyWarningOpts)
GhcLibHcOpts    = -O -dcore-lint $(MyWarningOpts)
GhcLibWays      =
GhcStage1HcOpts = -Rghc-timing -O -fasm
GhcStage2HcOpts = -Rghc-timing -O0 -DDEBUG
SplitObjs = NO

endif

# -----------------------------------------------------------------------------
# Other settings that might be useful

# profiled RTS
#GhcRtsCcOpts =  -pg -g

# Optimised/profiled RTS
#GhcRtsCcOpts = -O2 -pg

#GhcRtsWithFrontPanel = YES
#SRC_HC_OPTS += `gtk-config --libs`

# NoFib settings
NoFibWays =
STRIP=:
