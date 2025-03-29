# use clang-format to format the code
format:
	find . -name "*.cpp" -o -name "*.h" | xargs clang-format -i -style=file
