# Compile-time tests — compilation = test pass
add_executable(le_helpers_test compile_time/le_helpers_test.cpp)
target_link_libraries(le_helpers_test PRIVATE ble_sniffer)

# Runtime assertion tests — return 0 on success, 1 on failure
add_executable(byte_order_test compile_time/byte_order_test.cpp)
target_link_libraries(byte_order_test PRIVATE ble_sniffer)
add_executable(airpods_label_test compile_time/airpods_label_test.cpp)
target_link_libraries(airpods_label_test PRIVATE ble_sniffer)
