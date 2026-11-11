MAIN_OUT = main

TEST_OUT = test_eval

SRC_DEPS += $(wildcard include/sat/*.h)
SRC_DEPS += $(wildcard cmd/*.h)

include mk.tpl

CXXFLAGS += -Iinclude

# Main is tested
$(eval $(call TEST_template,${MAIN_OUT}))

# TEST is tested
$(eval $(call CMD_template,${TEST_OUT}))
$(eval $(call TEST_template,${TEST_OUT}))
