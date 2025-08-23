from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps

class Shapes(ConanFile):
    name = "Shapes"
    version = "0.0.1"

    author = "dandistine"
    description = "OLC Game Jam 2025"

    settings =  "os", "compiler", "build_type", "arch"

    exports_sources = "CmakeLists.txt", "include/*", "src/*"

    def requirements(self):
        #self.test_requires("gtest/1.16.0")
        self.requires("olcpixelgameengine/2.29")
        self.requires("simple_serialization/0.0.1")
        self.requires("pcg-cpp/cci.20220409")
        self.requires("entt/3.14.0")
        self.requires("tinyxml2/11.0.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        #cmake.ctest()

    def package(self):
        cmake = CMake(self)
        cmake.install()