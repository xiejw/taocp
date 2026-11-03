use sat::eval::{Bench, ID};

fn main() {
    let id = ID::Waerden339;
    let bench = Bench::get(id);
    dbg!(id.human_str());
    dbg!(bench);
}
