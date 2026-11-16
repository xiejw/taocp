MODS    += ${BUILD_OBJS}/log.o
MODS    += ${BUILD_OBJS}/sat_literal.o
MODS    += ${BUILD_OBJS}/sat_backtrack.o

MAIN_OUT = main

TEST_OUT = test_main

include mk.tpl

CXXFLAGS += -I../../sat/include

CXXFLAGS += -Wno-unused-label

# Main is in `make test`
$(eval $(call TEST_template,${MAIN_OUT}))

# TEST is in `make test`
$(eval $(call CMD_template,${TEST_OUT}))
$(eval $(call TEST_template,${TEST_OUT}))

