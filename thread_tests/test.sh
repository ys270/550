for i in {1..100}
do
    echo "Test $i th time"
    ./thread_test_measurement
done
echo "Done!"
