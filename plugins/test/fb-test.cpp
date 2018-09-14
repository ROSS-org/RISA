#include "../../include/plugins/flatbuffers/data_sample_generated.h"
#include "flatbuffers/minireflect.h"
#include <cstdio>
#include <iostream>
#include <fstream>

// build: g++ -I /home/rossc3/flatbuffers/include/ -std=c++11 -g fb-test.cpp
using namespace std;
using namespace ross_damaris::sample;

int main(int argc, char *argv[])
{
    int buf_size = 262114;
	char *data = new char[buf_size];
    flatbuffers::uoffset_t length;
	streampos total_size;
    streampos pos = 0;
	
	ifstream file(argv[1], ios::in | ios::binary | ios::ate);
	if (file.is_open())
	{
        total_size = file.tellg();
        file.seekg(0, ios::beg);
        while (pos < total_size)
        {
            file.read(reinterpret_cast<char*>(&length), sizeof(length));
            if (length > buf_size)
            {
                cerr << "Error: length of " << length << " bytes is larger than default buffer size!" << endl;
                return 1;
            }

            file.read(data, length);
            pos = file.tellg();
            cout << "read " << length << " bytes from file" << endl;

            auto data_sample = GetDamarisDataSample(data);
            DamarisDataSampleT ds;
            data_sample->UnPackTo(&ds);

            flatbuffers::FlatBufferBuilder fbb;
            auto new_samp = DamarisDataSample::Pack(fbb, &ds);
            fbb.Finish(new_samp);
            auto s = flatbuffers::FlatBufferToString(fbb.GetBufferPointer(), DamarisDataSampleTypeTable(), true);
            cout << s << endl;
        }

	}


	

    return 0;
}
