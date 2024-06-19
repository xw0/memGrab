#include "program.h"
#include "imgui.h"
#include "systemFunctions.h"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <format>

#undef min
#undef max

void OpenProgram::render()
{
	//ImGui::Text("Process name");
	//bool open = ImGui::InputText("##Enter process name", processName, sizeof(processName), ImGuiInputTextFlags_EnterReturnsTrue);

	if (ImGui::BeginTabBar("##open selector"))
	{
		PID selectedP = 0;
		std::string processName = "";
		bool open = 0;

		if (ImGui::BeginTabItem("process name"))
		{
			processName = printAllProcesses(selectedP);
			if (ImGui::Button("open"))
			{
				open = true;
			}
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("window"))
		{
			processName = printAllWindows(selectedP);
			if (ImGui::Button("open"))
			{
				open = true;
			}
			ImGui::EndTabItem();
		}

		if (open)
		{
			fileOpenLog.clearError();

			//pid = findPidByName(processName);z
			pid = selectedP;

			if (pid == 0)
			{
				fileOpenLog.setError("No process selected");
			}
			else
			{
				strcpy(currentPocessName, processName.c_str());
				handleToProcess = openProcessFromPid(pid);

				if (handleToProcess == 0)
				{
					std::string s = "couldn't open process\n";
					s += getLastErrorString();
					fileOpenLog.setError(s.c_str());
					pid = 0;
				}
			}
		}

		ImGui::EndTabBar();
	}

	ImGui::Separator();

}

void SearchForValue::clear()
{
	foundValues.clear();
	currentItem = 0;
	data = {};
	str = {};
}

void *SearchForValue::render(PROCESS handle)
{
	ImGui::PushID(handle);

	ImGui::Text("search for value");

	bool changed = 0;
	typeInput<__COUNTER__ + 1000>(data, 0, &changed, &str);
	
	void* foundPtr = 0;

	if (changed)
	{
		foundValues.clear();
	}

	if (ImGui::Button("search"))
	{
		if (foundValues.empty())
		{
			if (data.type == Types::t_string)
			{
				foundValues = findBytePatternInProcessMemory(handle, (void*)str.c_str(), str.length());
			}
			else
			{
				foundValues = findBytePatternInProcessMemory(handle, data.ptr(), data.getBytesSize());
			}
		}
		else
		{
			if (data.type == Types::t_string)
			{
				refindBytePatternInProcessMemory(handle, (void*)str.c_str(), str.length(), foundValues);
			}
			else
			{
				refindBytePatternInProcessMemory(handle, data.ptr(), data.getBytesSize(), foundValues);
			}

		}

	}
	ImGui::SameLine();
	if (ImGui::Button("clearSearch"))
	{
		foundValues.clear();
	}
	
	if (!foundValues.empty())
	{
		auto valueGetter = [](void* data, int index, const char** result) -> bool {
			SearchForValue* _this = (SearchForValue*)data;

			if (index < 0 || index >= _this->foundValues.size()) {
				return false;
			}

			std::stringstream ss;
			ss << std::hex << ((unsigned long long)_this->foundValues[index]);
			_this->foundValuesTextMap[index] = ss.str();

			*result = _this->foundValuesTextMap[index].c_str();
			return true;
		};

		ImGui::Text("Found pointers: %d", (int)foundValues.size());
		ImGui::ListBox("##found pointers", &currentItem, valueGetter, this, foundValues.size(), std::min((decltype(foundValues.size()))10ull, foundValues.size()));
		
		if (currentItem < foundValues.size())
		{
			foundPtr = foundValues[currentItem];
		}

		if (foundPtr)
		{
			// refresh current item
			std::stringstream ss;
			ss << std::hex << ((unsigned long long)foundValues[currentItem]);
			foundValuesTextMap[currentItem] = ss.str();

			if (ImGui::Button(std::string("Copy: " + foundValuesTextMap[currentItem]).c_str()))
			{

			}else
			{
				foundPtr = 0;
			}
		}

	}

	ImGui::PopID();

	return foundPtr;
}

void SearchForInt32::clear()
{
	foundValues.clear();
	currentItem = 0;
}

void* SearchForInt32::render(PROCESS handle)
{
	ImGui::PushID(handle);

	ImGui::Text("search for int32");

	bool changed = 0;

	static int32_t minValue = 100;
	static int32_t maxValue = 1200;

	ImGui::InputInt("Min", &minValue);
	ImGui::InputInt("Max", &maxValue);

	void* foundPtr = 0;

	if (changed)
	{
		foundValues.clear();
	}

	if (ImGui::Button("Search"))
	{
		foundValues = findInt32InProcessMemory(handle, minValue, maxValue);
	}

	if (ImGui::Button("Refine / Decreased")) 
	{
		foundValues = refindDecreasedInt32InProcessMemory(handle, minValue, maxValue, foundValues);
	}

	if (ImGui::Button("Refine / Increased"))
	{
		foundValues = refindIncreasedInt32InProcessMemory(handle, minValue, maxValue, foundValues);
	}

	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		foundValues.clear();
	}

	if (!foundValues.empty())
	{
		auto valueGetter = [](void* data, int index, const char** result) -> bool {
			SearchForInt32* _this = (SearchForInt32*)data;

			if (index < 0 || index >= _this->foundValues.size()) {
				return false;
			}

			void* ptr = _this->foundValues[index].first;
			int32_t value = _this->foundValues[index].second;

			std::string output = std::format("0x{:016x} ({})", reinterpret_cast<std::uintptr_t>(ptr), value);
			_this->foundValuesTextMap[index] = output;

			*result = _this->foundValuesTextMap[index].c_str();
			return true;
			};

		ImGui::Text("Found pointers: %d", (int)foundValues.size());
		ImGui::ListBox("##found pointers", &currentItem, valueGetter, this, foundValues.size(), std::min((decltype(foundValues.size()))10ull, foundValues.size()));

		if (currentItem < foundValues.size())
		{
			foundPtr = foundValues[currentItem].first;
		}

		if (foundPtr)
		{
			// refresh current item
			void* ptr = foundValues[currentItem].first;
			int32_t value = foundValues[currentItem].second;

			std::string output = std::format("0x{:016x} ({})", reinterpret_cast<std::uintptr_t>(ptr), value);
			foundValuesTextMap[currentItem] = output;

			if (ImGui::Button(std::string("Copy: " + foundValuesTextMap[currentItem]).c_str()))
			{

			}
			else
			{
				foundPtr = 0;
			}
		}
	}

	ImGui::PopID();

	return foundPtr;
}

bool OppenedProgram::render()
{
	std::stringstream s;
	s << "Process: ";
	s << currentPocessName << "##" << pid << "open and use process";
	bool oppened = 1;
	if (ImGui::Begin(s.str().c_str(), &oppened, ImGuiWindowFlags_NoSavedSettings))
	{

		if (pid != 0 && isOppened)
		{
			ImGui::PushID(pid);
			//ImGui::Begin(currentPocessName);

			ImGui::Text("Process id: %d, name: %s", (int)pid, currentPocessName);
			static int v = 0;
			ImGui::NewLine();

			//ImGui::Text("Search for a value");
			//typeInput<__COUNTER__>();

			static void *memLocation = {};
			ImGui::NewLine();
			ImGui::Text("Enter memory location");
			ImGui::InputScalar("##mem location", ImGuiDataType_U64, &memLocation, 0, 0, "%016" IM_PRIx64, ImGuiInputTextFlags_CharsHexadecimal);
			ImGui::NewLine();

		#pragma region write to memory
			{
				ImGui::Text("Write to memory");
				bool enterPressed = 0;
				static std::string s;
				static GenericType data;
				typeInput<__COUNTER__>(data, &enterPressed, 0, &s);
				ImGui::SameLine();
				enterPressed |= ImGui::Button("Write");

				static bool lockMemory = false;
				ImGui::Checkbox("Lock", &lockMemory);
				enterPressed |= lockMemory;

				if (enterPressed)
				{
					if (data.type == Types::t_string)
					{
						writeMemory(handleToProcess, (void *)memLocation, (void *)s.c_str(), s.size(), writeLog);
					}
					else
					{
						writeMemory(handleToProcess, (void *)memLocation, data.ptr(), data.getBytesSize(), writeLog);
					}
				}

				writeLog.renderText();

				ImGui::NewLine();
			}
		#pragma endregion

			auto searched = searcher.render(handleToProcess);

			if (searched)
			{
				memLocation = searched;
			}

			//ImGui::End();
			ImGui::PopID();
		}
		else
		{
			ImGui::Text("Process name: %s", currentPocessName);
		}

		errorLog.renderText();

	}

	ImGui::End();


#pragma region hex
	if (0)
	{
		s << "Hex";

		if (ImGui::Begin(s.str().c_str(), &oppened, ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::PushID(pid);
			if (pid != 0 && isOppened)
			{
				//ImGui::Text("hex editor goes here");

				ImGui::BeginGroup();

				for (int i = 0; i < 10; i++)
				{

					for (int i = 0; i < 16; i++)
					{
						ImGui::Text("%c%c ", 'a' + i, 'a' + i);
						if (i < 15) { ImGui::SameLine(); }
					}

				}

				hexLog.setError("warn", ErrorLog::ErrorType::warn);

				ImGui::EndGroup();

			}
			else
			{
				ImGui::Text("Process name: %s", currentPocessName);
			}

			hexLog.renderText();

			ImGui::NewLine();

			ImGui::PopID();
		}

		ImGui::End();
	}

#pragma endregion


	return oppened;
}

void OppenedProgram::close()
{
	closeProcess(handleToProcess);
	isOppened = 0;
	handleToProcess = 0;
	searcher.clear();
}

bool OppenedProgram::isAlieve()
{
	if (!pid) { return false; }
	if (!isOppened) { return false; }

	if (isProcessAlive(handleToProcess))
	{
		return true;
	}
	else
	{
		return false;
	}
}
