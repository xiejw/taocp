//! Benchmark problems for evaluating SAT solvers.

/// Identifies a benchmark problem.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum ID {
    /// Rivest's unsatisfiable clauses R (TAOCP V4 7.2.2.2).
    Rivest,
    /// waerden(3,3;9) (TAOCP V4 7.2.2.2).
    Waerden339,
}

impl ID {
    /// Returns a human-readable name for printing.
    pub fn human_str(&self) -> &'static str {
        match self {
            ID::Rivest => "rivest",
            ID::Waerden339 => "waerden(3,3;9)",
        }
    }
}

/// A benchmark problem.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Bench {
    pub num_vars: usize,
}

impl Bench {
    /// Returns the benchmark problem identified by `id`.
    pub fn get(id: ID) -> Bench {
        match id {
            ID::Rivest => Bench { num_vars: 4 },
            ID::Waerden339 => Bench { num_vars: 9 },
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn get_returns_num_vars() {
        assert_eq!(Bench::get(ID::Rivest).num_vars, 4);
        assert_eq!(Bench::get(ID::Waerden339).num_vars, 9);
    }

    #[test]
    fn human_str() {
        assert_eq!(ID::Rivest.human_str(), "rivest");
        assert_eq!(ID::Waerden339.human_str(), "waerden(3,3;9)");
    }
}
