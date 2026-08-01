MODS     += src/v4/v4_7.1.1.algo_c_core_computation_for_definite_horn
MODS     += src/v4/v4_7.1.1.theo_k_2sat_krom_clauses
MODS     += src/v4/v4_7.2.2.algo_b_basic_backtrack
MODS     += src/v4/v4_7.2.2.algo_b_basic_backtrack_bit_vec
MODS     += src/v4/v4_7.2.2.algo_w_walker_backtrack
MODS     += src/v4/v4_7.2.2.1.algo_x_exact_cover_via_dancing_links
MODS     += src/v4/v4_7.2.2.2.algo_b_satisfiability_by_watching
#MODS     += src/v4/v4_7.2.2.2.algo_c_satisfiability_by_cdcl
#MODS     += roadmap/v4_7.2.2.4.algo_h_all_hamiltonian_cycles
MODS     += src/v4/v4_7.4.1.2.algo_t_strong_components

# MODS     += v4_sat

compile:
	@set -e; for m in $(MODS); do                             \
		make --no-print-directory -C $$m compile;         \
		done

test: test_forge_version
ifdef ASAN
	@set -e; for m in $(MODS); do                             \
		make --no-print-directory -C $$m ASAN=1 test;     \
		done
else
	@set -e; for m in $(MODS); do                             \
		make --no-print-directory -C $$m test;            \
		done
endif

fmt:
	@set -e; for m in $(MODS); do                             \
		make --no-print-directory -C $$m fmt;             \
		done

clean:
	@set -e; for m in $(MODS); do                             \
		make --no-print-directory -C $$m clean;           \
		done

release:
	@set -e; for m in $(MODS); do                             \
		make --no-print-directory -C $$m release;         \
		done

test_forge_version:
	go run tools/forge_version_tag_checker.go
