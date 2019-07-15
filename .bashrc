alias buildenv="docker run -v $(pwd):/project --workdir /project sw-timer-buildenv"
alias buildenv-init="docker build -t sw-timer-buildenv ."
# Someone will typo this anyway..
alias buildenv-make="buildenv make"

echo "Aliases created."
echo "To create the build environment, execute 'buildenv-init'."
echo "To build the application, run 'buildenv make'."
