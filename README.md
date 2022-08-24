# A unit testing framework for OpenFOAM code

This is a unit/integration testing framework to help test-proof new OpenFOAM code (might be too late for the OpenFOAM library
itself). Master branch works with Foam-Extend 4 (because that's what I'm dealing with mostly, but can be adapted to other forks
easily).

> See example reports at the bottom!

## How to use this repository

- Add it to your library repo as a submodule or a subtree somewhere under your tests tree
```bash
git subtree add --prefix tests https://github.com/FoamScience/OpenFOAM-Unit-testing.git master --squash
```

- Get familiarized with the directory structure:
```bash
.
├── Alltest         # A script to run all tests and produce reports
├── cases           # A directory to hold all OpenFOAM test cases
├── catch2          # Catch2 subtree
├── include         # Helpers for unit/integration test writers
├── README.md       # This README
├── reports         # This is where the HTML reports get generated
├── style           # Style files to decorate the HTML reports
└── tests           # The code testing the "tests" library (examples)
```

- Here is how the tests directory should look
```bash
tests
├── Make                     # Make/files should compile all *C files
├── serialParallelTests      # Directory for different *C tests (e.g. a directory per tested class)
├── testsTestDriver.C        # Test driver for the tested library "tests"
                             # Resulting binary has to be named ${libName}TestDriver for the Alltest script to work
```

- Try to add a test file to `tests/serialParallelTests` (and mention in `Make/files`) then run the `Alltest` script.

## Design principles

- Tests are created and ran with [Catch2 v3.x](https://github.com/catchorg/Catch2)
    - [Catch2 docs](https://github.com/catchorg/Catch2/tree/devel/docs)
      and [Phil's talk at CppCon 2018](https://github.com/catchorg/Catch2/tree/v2.x/docs)
      are the best places to learn more
- [FakeIt](https://github.com/eranpeer/FakeIt) (also Header-only) is used for mocking.
    - Header named `fakeit.H`
- Each library you want to test has its own test driver
    - Convention: The name of **the test binary** should end with `TestDriver`
    - Each class's tests suite should be written in a separate `.C` file
- The test driver acts like an OpenFOAM solver and must run on an OpenFOAM case
  directory. For a fully-in-memory testing setup, it's enough to move the test case
  to `/dev/shm` on most systems.
  - `mpirun -np 2 ./TestDriver -p` or `mpirun -np 2 ./TestDriver --parallel` will work for parallel runs.
  - The provided template for the test driver need to be invoked from the case directory (has no support for `-case` flag).
- The same unit-test case can check for results both in serial and in parallel.
- Tagging is used to distinguish between serial/parallel tests, and to specify which OpenFOAM case to use to run the tests.
- Supported reporters:
    - `stdout` for output at the console (Best for local and CI testing)
    - `compact` for status overview in a single line
    - `xml` and `junit` for Data collectors

## Why write (Unit) tests?

- To make sure new functionality works as expected
- To make sure further development does not introduce bugs, or change older
  behaviour (Unintentionally).
- To better document the expected usage of code.
- To gauge performance if needed.

## Best practices and guidelines for tests

### General guidelines

- Simple `TEST_CASE`s with `SECTION`s are preferred (can't generate HTML reports for TDD-style tests yet).
- Avoid relying on dictionaries stored on disk; It's recommended to **build the
  dictionaries programmatically** immediately before using them and never
  writing time directories to disk (Because a single case will be used for multiple
  tests).
- Each test should be manually tagged for the following
  (Task left to the use for now):
    - `[Integration]` or `[Unit]` for integration and unit tests respectively
    - `[Parallel]` for tests which are meant to test parallel functionality,
      or `[Serial]` otherwise.
    - `[Case_caseName]` will run the test unit on the OpenFOAM case `cases/caseName`.
- OpenFOAM results should be in ASCII uncompressed format (if results need to be written to disk).
- The main focus is to test the interface of your classes (i.e. its public parts)
    - But if there is a need to test a private method, use macros defined in `include/memberStealer.H`
- Use `CAPTURE()` to record useful case parameters (which will show up in reports).
- **Standard output** is consumed by Catch2, print to standard error to see
  the logs at the console (Or use `Pout`), but doing so will mess up report generation.
- Some convenient macros to create time, mesh and field objects can be found in `include/testMacros.H`

### Strongly recommended testing guidelines

> Break these guidelines and you'll have a hard time debugging the tests!

1. Create only **ONE** Time object at the start of each test,
   at global scope; enclosed in a unique namespace. For example:
   ```cpp
   namespace serialParallel // Arbitrary name
   {
       // Requirement 0.1: Time for parallel runs (only if you need it obviously)
       #include "createTestTime.H"
       
       // Requirement 0.1: Pointer for mesh (again if mesh is needed)
       autoPtr<fvMesh> meshPtr;
   }
   using namespace serialParallel;
   TEST_CASE(" ... ") { ... }
   ```

2. As the previous example outlined, it's recommended to have a single
   pointer to a (**concrete**) mesh class, which we then reset over and
   over:
   ```cpp
   namespace serialParallel
   {
       #include "createTestTime.H"
       Foam::autoPtr<Foam::fvMesh> meshPtr;
   }
   using namespace serialParallel;

   TEST_CASE(" ... ")
   {
        // Generate a variable, test case runs 10 times
        auto number = GENERATE( range(1,10) );

        // Reset the mesh (macro from testMacros.H)
        resetMeshPointer
        (
            runTime, // The time object
            meshPtr, // Pointer name
            fvMesh, // Mesh class
            fvMesh::defaultRegion // Mesh region
        );
        auto& mesh = meshPtr();

        // Do what's needed, and
        meshPtr->clear();
   }
   ```
> Global pointers are hated for a resounding reason;
> Never try to access these mesh pointers from other files please!

Parallel tests are tricky when you want to use advanced Catch2 features,
especially `GENERATORS`; which want the time object to be in global scope,
but MPI comms don't work for global variables (So, `Pstream::myProcNo()`
will always evaluate to 0 in global scope).

The `memberStealer` template becomes very useful in overcoming this
problem. By overwriting `processorCase_` and `case_` in the time object,
we can point it to the correct processor directory inside the test case
(Note the `prepareTimePaths()` call at global scope):

```cpp
namespace serialParallel
{
    #include "createTestTime.H"
    autoPtr<fvMesh> meshPtr;
}
using namespace serialParallel;

// This is not necessary for serial runs, but gives access to some private
// members of the time object
prepareTimePaths();

TEST_CASE(...)
{
    if (Pstream::parRun()) {
        // If parallel run, fix case paths - assuming you ran the binary from the case
        // directory (which is the only supported way for now)
        word procPath = "processor"+Foam::name(Pstream::myProcNo());
        modifyTimePaths(runTime, true, procPath);
    }

    // Generate some parameter values (4 exclusive)
    auto param = GENERATE( range(1,4) );
    
    // Depending on your MPI code, you might need to call Pstream::waitRequests()
    // for each generated section  of the test
}
```

## Catch2 Output

On failure, Catch2 displays a message containing `TEST_CASE` name,
or the BDD logic (SCENARIO, GIVEN, WHEN, THEN) with the captured variables.

> With parallel runs, this information is printed for each processor which
> has some tests running

If you pass `-s` to the test driver, the same information is printed
for successful tests too:
```
-------------------------------------------------------------------------------
Support for serial and parallel runs
  Decomposed mesh in parallel runs
-------------------------------------------------------------------------------
serialParallelTests/serialParalleTest.C:67
...............................................................................

serialParallelTests/serialParalleTest.C:80: PASSED:
  REQUIRE( mesh.nCells() < gNCells )
with expansion:
  1318 (0x526) < 2636 (0xa4c)
with messages:
  mesh.nCells() := 1318 (0x526)
  gNCells := 2636 (0xa4c)
  Pstream::parRun() := true
```

## Integration with CI and reporting

> By default, all tests run in memory (`/dev/shm`), change `caseRun` variable in `./Alltest` to your desired path.

You can change the flags passed to the test driver in `Alltest` to suit your needs.
For example, 
```bash
# To generate JUnit XML reports
$root/$lib/${lib}TestDriver -s -r junit -o serialTests.xml "[Serial][Case_$caseName]"
```

`Alltest.report` provides an example of such report generation (with a custom HTML template).

Here is an example HTML report from a parallel test run (produced with `./Alltest.report`:

![file](https://user-images.githubusercontent.com/34474472/186402299-7f3cec00-0572-4a7b-a3f6-60df2e7dd015.svg)
