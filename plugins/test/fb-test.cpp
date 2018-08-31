#include "../include/schemas/data_sample_generated.h"
#include "flatbuffers/minireflect.h"
#include <cstdio>
#include <iostream>
#include <fstream>

using namespace std;
using namespace ross_damaris::sample;

int main()
{
    int buf_size = 8192;
	char *data = new char[buf_size];
	int length;
    //flatbuffers::uoffset_t length;
	streampos total_size;
    streampos pos = 0;
	
	ifstream file("/home/rossc3/rv-build/models/phold/test-fb.bin", ios::in | ios::binary | ios::ate);
	//ifstream file("../nettest.bin", ios::in | ios::binary | ios::ate);
	if (file.is_open())
	{
        total_size = file.tellg();
        file.seekg(0, ios::beg);
        while (pos < total_size)
        {
            file.read(reinterpret_cast<char*>(&length), sizeof(length));
            if (length > buf_size)
                cout << "length is larger than default buffer size!" << endl;
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
