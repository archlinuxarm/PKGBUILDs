# Ruby Packaging

## Bootstrap

To transition to a new language version of Ruby like 3.2 to 3.3, the process
involves three main phases: initial bootstrapping with a non-de-vendored Ruby,
rebuilding a base set of Ruby packages using a production build, and finally, a
comprehensive rebuild of all existing Ruby-dependent packages in the correct
sequence.

### bundled gem compatibility

A list of compatible bundled gem versions for packaging can be found at
<https://stdgems.org>. The de-vendored packages should make sure to be
compatible to the ruby version.

### bootstrap packages

The bootstrapping phase starts with the creation of a minimal set of bootstrap
packages, crucially including a non-de-vendored version of Ruby. This step aims
to establish a foundational Ruby environment that is capable of supporting the
compilation and installation of more complex Ruby applications and libraries.
It’s essential that this version of Ruby is non-de-vendored to ensure that all
components required for further package dependencies are available and
functional.

Build the following packages in the given order:

- ruby `_bootstrap=1 pkgrel=0`
- rubygems `--nocheck`
- ruby-rake
- ruby-hoe
- ruby-kpeg
- ruby-rdoc
- ruby-irb
- ruby-power_assert `--nocheck`
- ruby-test-unit
- ruby-webrick
- ruby-erb `--nocheck`

### base set

Following the initial setup, the next step involves the rebuilding of a
specific chain of Ruby packages using a production build of Ruby. This
production build should be a de-vendored version of Ruby, intended for
deployment. The purpose of this phase is to compile a full base set of Ruby
packages that are ready for consumption. This includes key libraries and tools
necessary for the broader Ruby ecosystem.

Build the following packages in the given order:

- ruby `_bootstrap=0`
- rubygems
- ruby-rake
- ruby-bundler
- ruby-webrick
- ruby-rake-compiler
- ruby-erb
- ruby-power_assert
- ruby-test-unit
- ruby-diff-lcs
- ruby-rspec-support
- ruby-rspec-core
- ruby-rspec-expectations
- ruby-rspec-mocks
- ruby-rspec
- ruby-rr `--nocheck`
- ruby-test-unit-rr
- ruby-rr
- ruby-debug
- ruby-irb
- ruby-minitest
- ruby-hoe
- ruby-kpeg
- ruby-racc
- ruby-rdoc
- ruby-typeprof

### rebuild all packages

The final step in upgrading to a new language version of Ruby is the comprehensive
rebuild of all existing packages that depend on Ruby, in their appropriate
sequential order. This step is critical to ensure that all applications and
libraries are compatible with the new Ruby version. This rebuild must be
carefully managed to maintain dependency integrity.

- rebuild all packages in order
