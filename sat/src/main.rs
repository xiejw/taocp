use sat::eval::{Bench, ID};

fn main() {
    let id = ID::Waerden339;
    let bench = Bench::get(id);
    dbg!(bench.info);
}
