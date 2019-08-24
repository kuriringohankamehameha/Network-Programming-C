ifeq (run,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_ARGS):;@:)
endif

prog: load_balancer
	gcc -o load_balancer load_balancer.c

.PHONY: run
run : load_balancer
	./load_balancer $(RUN_ARGS)
