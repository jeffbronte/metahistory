TARGET=h
CPPVER='-std=c++11'
DEBUG='-g'

$(TARGET): metahistory.cpp
	$(CXX) $(CPPVER) $(DEBUG) -Wall $^ -o $@

clean:
	rm -f $(TARGET)
