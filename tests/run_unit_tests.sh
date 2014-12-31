#Add new test files with dependencies to this var, separate with white space
TEST_SUITES=(
    "match_alg_tests.cpp ../src/MatchAlgorithm.cpp"
)

#--------------#
CXX=g++
OUTPUT_BIN=tests

for unit in "${TEST_SUITES=[@]}"
do 
    rm -f $OUTPUT_BIN
    echo "$CXX $unit -lboost_unit_test_framework -o $OUTPUT_BIN -DNO_WT"
    $CXX $unit -lboost_unit_test_framework -o $OUTPUT_BIN -DNO_WT
	if [ $? -ne 0 ]
	then
    	echo "Compilation of files: $unit failed!"
    	exit 1
	fi    
    ./$OUTPUT_BIN
    rm -f $OUTPUT_BIN
done 
#--------------#
