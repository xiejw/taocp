MAIN_OUT = main

TEST_OUT = test_eval

include mk.tpl

# Main is tested
$(eval $(call TEST_template,${MAIN_OUT}))

# TEST is tested
$(eval $(call CMD_template,${TEST_OUT}))
$(eval $(call TEST_template,${TEST_OUT}))
