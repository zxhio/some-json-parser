fn main() {
    let mut r = jsonrs::Reader::new("{}");
    println!("{:?}", r.parse());
}

