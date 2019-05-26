#include <iostream>
#include "../../include/angelscript.h"
#include <string>
#include "assert.h"

class BytecodeStream : public asIBinaryStream
{
public:
	BytecodeStream() : m_buffer(nullptr), m_size(0), m_capacity(0), m_readIndex(0)
	{}

	int Write(const void* ptr, asUINT size)
	{
		size_t newSize = size + m_size;
		if (newSize > m_capacity)
		{
			do
			{
				m_capacity += 512;
			} while (newSize > m_capacity);

			char* newBuffer = new char[m_capacity];
			if (m_buffer)
			{
				memcpy(newBuffer, m_buffer, m_size);
				delete m_buffer;
			}
			m_buffer = newBuffer;
		}

		if (size == 0) return 0;

		memcpy(m_buffer + m_size, ptr, size);
		m_size += size;

		return 0;
	}
	int Read(void* ptr, asUINT size)
	{
		if (size == 0) return 0;

		memcpy(ptr, m_buffer + m_readIndex, size);
		m_readIndex += size;
		return 0;
	}

	char* m_buffer;

	size_t m_readIndex;
	size_t m_size;
	size_t m_capacity;
};


int CompileScript1(asIScriptEngine* engine, const char* moduleName, BytecodeStream& stream);
int CompileScript2(asIScriptEngine* engine, const char* moduleName, BytecodeStream& stream);


int main(int argCount, char* argVal[])
{
	asIScriptEngine* engine = asCreateScriptEngine();
	if (engine == 0)
	{
		std::cout << "Failed to create script engine." << std::endl;
		return -1;
	}

	int r = 0;

	BytecodeStream stream1;
	BytecodeStream stream2;

	CompileScript2(engine, "test2", stream2);
	CompileScript1(engine, "test1", stream1);

	asIScriptModule* mod2 = engine->GetModule("test2", asGM_ALWAYS_CREATE);
	r = mod2->LoadByteCode(&stream2);
	assert(r >= 0);

	//invalid bytecode with outdated shared class
	asIScriptModule* mod1_1 = engine->GetModule("test1_1", asGM_ALWAYS_CREATE);
	r = mod1_1->LoadByteCode(&stream1);
	assert(r < 0);

	//just need another compilation to trigger "engine->signatureIds" array search
	asIScriptModule* mod1_2 = engine->GetModule("test1_2", asGM_ALWAYS_CREATE);
	stream1.m_readIndex = 0;
	r = mod1_2->LoadByteCode(&stream1);
	assert(r < 0);


	return 0;
}

int CompileScript1(asIScriptEngine* engine, const char* moduleName, BytecodeStream& stream)
{
	int r;

	const char* file1 = ""
		"shared class Test1 { \n"
		//mismatching functions
		"	int function1() { return 0; } \n"
		""
		"}\n"
		;

	asIScriptModule* mod = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("script1", file1, strlen(file1));
	assert(r >= 0);

	r = mod->Build();
	assert(r >= 0);

	r = mod->SaveByteCode(&stream);
	assert(r >= 0);

	mod->Discard();

	return 0;
}


int CompileScript2(asIScriptEngine* engine, const char* moduleName, BytecodeStream& stream)
{
	int r;

	const char* file2 = ""
		"shared class Test1 { \n"
		//mismatching functions
		"	int function2() { return 0; } \n"
		""
		"}\n"
		;

	asIScriptModule* mod = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection(moduleName, file2, strlen(file2));
	assert(r >= 0);

	r = mod->Build();
	assert(r >= 0);

	r = mod->SaveByteCode(&stream);
	assert(r >= 0);

	mod->Discard();

	return 0;
}