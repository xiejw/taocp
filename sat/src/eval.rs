//! Benchmark problems for evaluating SAT solvers.

// === --- Literal --------------------------------------------------------- ===
//

/// A literal: `Literal(k)` is x_k and `Literal(-k)` is ¬x_k, with k >= 1.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct Literal(pub isize);

impl Literal {
    /// Returns the positive literal x_v. `v` must be positive.
    pub fn new_pos(v: isize) -> Literal {
        assert!(v > 0, "var id must be positive, got {v}");
        Literal(v)
    }

    /// Returns the negative literal ¬x_v. `v` must be positive.
    pub fn new_neg(v: isize) -> Literal {
        assert!(v > 0, "var id must be positive, got {v}");
        Literal(-v)
    }

    /// Parses DIMACS notation: `k` means x_k and `-k` means ¬x_k. `l` must be
    /// nonzero.
    pub fn from_dimacs(l: isize) -> Literal {
        if l > 0 {
            Literal::new_pos(l)
        } else if l == 0 {
            panic!("DIMACS literal must be nonzero");
        } else {
            Literal::new_neg(-l)
        }
    }
}

// === --- Bench and ID ---------------------------------------------------- ===
//

/// Identifies a benchmark problem.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum ID {
    Rivest,
    Waerden339,
}

impl ID {
    /// Returns a human-readable name for printing.
    pub fn human_str(&self) -> &'static str {
        match self {
            ID::Rivest => "v4b;pg188;eq6;rivest",
            ID::Waerden339 => "v4b;pg188;eq9;waerden(3,3;9)",
        }
    }
}

/// A benchmark problem.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Bench {
    pub num_vars: usize,
    /// All literals of all clauses, laid out one clause after another.
    pub clauses: Vec<Literal>,
    /// Clause j is `clauses[begin..end]` where `(begin, end) = segments[j]`.
    pub segments: Vec<(usize, usize)>,
}

impl Bench {
    /// Returns the benchmark problem identified by `id`.
    pub fn get(id: ID) -> Bench {
        match id {
            ID::Rivest => Bench::from_clauses(
                4,
                &[
                    &[1, 2, -3],
                    &[2, 3, -4],
                    &[3, 4, 1],
                    &[4, -1, 2],
                    &[-1, -2, 3],
                    &[-2, -3, 4],
                    &[-3, -4, -1],
                    &[-4, 1, -2],
                ],
            ),
            ID::Waerden339 => Bench::waerden_3_3(9),
        }
    }

    /// Builds a `Bench` from clauses written in DIMACS notation: each literal
    /// is a nonzero integer, `k` for x_k and `-k` for ¬x_k (see
    /// `Literal::from_dimacs`).
    fn from_clauses(num_vars: usize, cs: &[&[isize]]) -> Bench {
        let mut clauses = Vec::new();
        let mut segments = Vec::with_capacity(cs.len());
        for c in cs {
            let begin = clauses.len();
            clauses.extend(c.iter().map(|&l| Literal::from_dimacs(l)));
            segments.push((begin, clauses.len()));
        }
        Bench {
            num_vars,
            clauses,
            segments,
        }
    }

    /// waerden(3,3;n): for every arithmetic progression i, i+d, i+2d in
    /// 1..=n, the clauses (x_i x_{i+d} x_{i+2d}) and (¬x_i ¬x_{i+d} ¬x_{i+2d}).
    /// All positive clauses come first, then all negative ones.
    fn waerden_3_3(n: isize) -> Bench {
        // Stage 1: generate all arithmetic progression (ap).
        //
        let mut aps = Vec::new();
        for d in 1..=(n - 1) / 2 {
            for i in 1..=n - 2 * d {
                aps.push([i, i + d, i + 2 * d]);
            }
        }

        // Stage 2: fill into clauses and segments
        // - 2 is due to poss and negs.
        // - 3 is due to the number of literals in each ap
        //
        let mut clauses = Vec::with_capacity(2 * 3 * aps.len());
        let mut segments = Vec::with_capacity(2 * aps.len());
        for new_lit in [Literal::new_pos, Literal::new_neg] {
            for ap in &aps {
                let begin = clauses.len();
                clauses.extend(ap.iter().map(|&v| new_lit(v)));
                segments.push((begin, clauses.len()));
            }
        }
        Bench {
            num_vars: n as usize,
            clauses,
            segments,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Brute-force: true if some assignment satisfies every clause.
    ///
    /// Tries all 2^n assignments, where n = `b.num_vars`. An assignment is a
    /// u64 `bits` whose bit k-1 holds the value of x_k (1 = true), so counting
    /// `bits` from 0 to 2^n - 1 visits every assignment exactly once.
    ///
    /// For each assignment, every clause (one `(begin, end)` segment) must be
    /// true; a clause is true if any of its literals is true. Literal x_k is
    /// true when bit k-1 is 1, and ¬x_k when that bit is 0. `any`/`all` stop
    /// early, so the search ends at the first satisfying assignment.
    ///
    /// Cost is O(2^n * total literals), which is fine for the small benchmarks
    /// here; it requires n <= 63 so `1 << n` fits in a u64.
    fn satisfiable_by_brute_force(b: &Bench) -> bool {
        (0u64..1 << b.num_vars).any(|bits| {
            b.segments.iter().all(|&(begin, end)| {
                b.clauses[begin..end].iter().any(|&Literal(l)| {
                    let v = bits >> (l.unsigned_abs() - 1) & 1 == 1;
                    if l > 0 { v } else { !v }
                })
            })
        })
    }

    #[test]
    fn get_returns_num_vars() {
        assert_eq!(Bench::get(ID::Rivest).num_vars, 4);
        assert_eq!(Bench::get(ID::Waerden339).num_vars, 9);
    }

    #[test]
    fn human_str() {
        assert_eq!(ID::Rivest.human_str(), "v4b;pg188;eq6;rivest");
        assert_eq!(ID::Waerden339.human_str(), "v4b;pg188;eq9;waerden(3,3;9)");
    }

    #[test]
    fn literal_new() {
        assert_eq!(Literal::new_pos(3), Literal(3));
        assert_eq!(Literal::new_neg(3), Literal(-3));
    }

    #[test]
    fn literal_from_dimacs() {
        assert_eq!(Literal::from_dimacs(3), Literal::new_pos(3));
        assert_eq!(Literal::from_dimacs(-3), Literal::new_neg(3));
    }

    #[test]
    #[should_panic(expected = "DIMACS literal must be nonzero")]
    fn literal_from_dimacs_rejects_zero() {
        Literal::from_dimacs(0);
    }

    #[test]
    #[should_panic(expected = "var id must be positive")]
    fn literal_new_pos_rejects_zero() {
        Literal::new_pos(0);
    }

    #[test]
    #[should_panic(expected = "var id must be positive")]
    fn literal_new_neg_rejects_negative() {
        Literal::new_neg(-2);
    }

    #[test]
    fn clause_counts() {
        let r = Bench::get(ID::Rivest);
        assert_eq!(r.segments.len(), 8);
        assert_eq!(r.clauses.len(), 24);
        let w = Bench::get(ID::Waerden339);
        assert_eq!(w.segments.len(), 32);
        assert_eq!(w.clauses.len(), 96);
    }

    #[test]
    fn segments_tile_clauses() {
        for id in [ID::Rivest, ID::Waerden339] {
            let b = Bench::get(id);
            let mut pos = 0;
            for &(begin, end) in &b.segments {
                assert_eq!(begin, pos);
                assert!(end > begin);
                pos = end;
            }
            assert_eq!(pos, b.clauses.len());
        }
    }

    #[test]
    fn all_unsatisfiable() {
        assert!(!satisfiable_by_brute_force(&Bench::get(ID::Rivest)));
        assert!(!satisfiable_by_brute_force(&Bench::get(ID::Waerden339)));
    }

    #[test]
    fn rivest_minus_last_clause_is_satisfiable() {
        let mut r = Bench::get(ID::Rivest);
        r.segments.pop();
        assert!(satisfiable_by_brute_force(&r));
    }
}
