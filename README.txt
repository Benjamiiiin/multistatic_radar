
# Dependencies

matplotlib:
    python -m pip install -U pip
    python -m pip install -U matplotlib


# Building the Program

To build the OpenMPI C program using the "build" directory, type the
following:

    cd build
    cmake ..
    make


# Running the Program

To run the simulation, navigate to the root directory of the project and
run the visualiser.py script. This generates the plot for visualising 
the results, and is also responsible for running the built MPI program
upon the "Simulate" button being pressed.
