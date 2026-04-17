from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class MmioConan(ConanFile):
    name = "mmio"
    version = "1.0.0"
    package_type = "header-library"
    license = "MIT"
    author = "LLevella"
    url = "https://github.com/LLevella/mmio"
    homepage = "https://github.com/LLevella/mmio"
    description = "Header-only C++17 Matrix Market I/O library"
    topics = ("matrix-market", "sparse-matrix", "coo", "csr", "csc")
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = (
        "CMakeLists.txt",
        "LICENSE",
        "README.md",
        "cmake/*",
        "include/*",
    )

    def layout(self):
        cmake_layout(self)

    def generate(self):
        CMakeDeps(self).generate()
        CMakeToolchain(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={"BUILD_TESTING": False})
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "mmio")
        self.cpp_info.set_property("cmake_target_name", "mmio::mmio")
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
