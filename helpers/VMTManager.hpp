#pragma once

#include <map>

typedef DWORD** PPDWORD;

class VFTableHook {
	VFTableHook(const VFTableHook&) = delete;
public:

	VFTableHook(PPDWORD ppClass, bool bReplace, bool bCopyRTTI = false) {
		m_ppClassBase = ppClass;
		m_bReplace = bReplace;
		m_bCopyRTTIInfo = bCopyRTTI;

		// No RTTI Info available
		if (bCopyRTTI && (((int)*ppClass) - 4) == NULL)
			m_bCopyRTTIInfo = false;

		if (bReplace) {
			m_pOriginalVMTable = *ppClass;
			uint32_t dwLength = CalculateLength();

			if (m_bCopyRTTIInfo)
			{
				m_pOriginalVMTable = (PDWORD)(((int)*ppClass) - 4);
				dwLength += 1;
			}

			m_pNewVMTable = new DWORD[dwLength];
			memcpy(m_pNewVMTable, m_pOriginalVMTable, dwLength * sizeof(DWORD));


			DWORD old;
			VirtualProtect(m_ppClassBase, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &old);
			*m_ppClassBase = (bCopyRTTI ? &m_pNewVMTable[1] : m_pNewVMTable);
			VirtualProtect(m_ppClassBase, sizeof(DWORD), old, &old);
		}
		else {
			m_pOriginalVMTable = *ppClass;
			m_pNewVMTable = *ppClass;
		}
	}
	~VFTableHook() {
		RestoreTable();
		if (m_bReplace && m_pNewVMTable) delete[] m_pNewVMTable;
	}

	void RestoreTable() {
		for (auto& pair : m_vecHookedIndexes) {
			Unhook(pair.first);
		}
	}

	template<class Type>
	Type Hook(uint32_t index, Type fnNew) {
		index += (m_bCopyRTTIInfo ? 1 : 0);
		DWORD dwOld = (DWORD)m_pOriginalVMTable[index];
		m_pNewVMTable[index] = (DWORD)fnNew;
		m_vecHookedIndexes.insert(std::make_pair(index, (DWORD)dwOld));
		return (Type)dwOld;
	}

	void Unhook(uint32_t index) {
		auto it = m_vecHookedIndexes.find(index);
		if (it != m_vecHookedIndexes.end()) {
			m_pNewVMTable[index] = (DWORD)it->second;
			m_vecHookedIndexes.erase(it);
		}
	}

	template<class Type>
	Type GetOriginal(uint32_t index) {
		return (Type)m_pOriginalVMTable[index];
	}

	template<class Type>
	static Type HookManual(uintptr_t* vftable, uint32_t index, Type fnNew) {
		DWORD Dummy;
		Type fnOld = (Type)vftable[index];
		VirtualProtect((void*)(vftable + index * 0x4), 0x4, PAGE_EXECUTE_READWRITE, &Dummy);
		vftable[index] = (uintptr_t)fnNew;
		VirtualProtect((void*)(vftable + index * 0x4), 0x4, Dummy, &Dummy);
		return fnOld;
	}

private:

	// c0unter, "nice" anti paste tho smh
	uint32_t CalculateLength()
	{
		uint32_t index = 0;
		MEMORY_BASIC_INFORMATION memory;
		if (!m_pOriginalVMTable) return 0;

		while (true)
		{
			if (VirtualQuery((void*)m_pOriginalVMTable[index], &memory, sizeof MEMORY_BASIC_INFORMATION) < 0 ||
				(memory.Protect != PAGE_EXECUTE
					&& memory.Protect != PAGE_EXECUTE_READ
					&& memory.Protect != PAGE_EXECUTE_READWRITE
					&& memory.Protect != PAGE_EXECUTE_WRITECOPY))
				break;

			index++;
		}

		return index;
	}

private:
	std::map<uint32_t, DWORD> m_vecHookedIndexes;

	PPDWORD m_ppClassBase;
	PDWORD m_pOriginalVMTable;
	PDWORD m_pNewVMTable;
	bool m_bReplace = false;
	bool m_bCopyRTTIInfo = false;
};