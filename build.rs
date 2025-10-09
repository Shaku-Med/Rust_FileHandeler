fn main() {
    println!("cargo:rustc-link-arg=-s");
    println!("cargo:rustc-link-arg=ALLOW_MEMORY_GROWTH=1");
    println!("cargo:rustc-link-arg=-s");
    println!("cargo:rustc-link-arg=EXPORTED_FUNCTIONS=[\"_malloc\",\"_free\"]");
}