.PHONY: build build-release clean install-deps test

install-deps:
	cargo install wasm-pack
	npm install

build:
	wasm-pack build --target web --out-dir pkg --out-name index

build-release:
	wasm-pack build --target web --out-dir pkg --out-name index --release

build-node:
	wasm-pack build --target nodejs --out-dir pkg-node --out-name index

build-bundler:
	wasm-pack build --target bundler --out-dir pkg-bundler --out-name index

clean:
	cargo clean
	rm -rf pkg pkg-node pkg-bundler

test:
	cargo test

all: install-deps build-release