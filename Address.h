#pragma once
#include <vector>
#include <windows.h>

template <typename T>
class Address
{
public:
	DWORD base;
	std::vector<DWORD> offset;
	bool is_readable();
	bool is_writeable();
	DWORD GetFinalAddress();
	T GetValue();
	bool SetValue(T);
	Address<T>& operator+(DWORD of);
	Address(DWORD bs);
};

template <class T>
bool Address<T>::is_readable()
{
	DWORD b = base;
	if (IsBadReadPtr((void*)b, 4))
		return false;
	for (int i = 0; i < offset.size(); i++)
	{
		b = *(DWORD*)b + offset[i];
		if (IsBadReadPtr((void*)b, 4))
			return false;
	}
	if (IsBadReadPtr((void*)b, sizeof(T)))
		return false;
	return true;
}
template <class T>
bool Address<T>::is_writeable()
{
	DWORD b = base;
	if (IsBadReadPtr((void*)b, 4))
		return false;
	for (int i = 0; i < offset.size(); i++)
	{
		b = *(DWORD*)b + offset[i];
		if (IsBadReadPtr((void*)b, 4))
			return false;
	}
	if (IsBadWritePtr((void*)b, sizeof(T)))
		return false;
	return true;
}
template <class T>
DWORD Address<T>::GetFinalAddress()
{
	DWORD b = base;
	if (IsBadReadPtr((void*)b, 4))
		return nullptr;
	for (int i = 0; i < offset.size(); i++)
	{
		b = *(DWORD*)b + offset[i];
		if (IsBadReadPtr((void*)b, 4))
			return nullptr;
	}
	return b;
}
template <class T>
T Address<T>::GetValue()
{
	DWORD b = base;
	if (IsBadReadPtr((void*)b, 4))
		return static_cast<T>(0);
	for (int i = 0; i < offset.size(); i++)
	{
		b = *(DWORD*)b + offset[i];
		if (IsBadReadPtr((void*)b, 4))
			return static_cast<T>(0);
	}
	if (IsBadReadPtr((void*)b, sizeof(T)))
		return static_cast<T>(0);
	return *(T*)b;
}
template <class T>
bool Address<T>::SetValue(T t)
{
	DWORD b = base;
	if (IsBadReadPtr((void*)b, 4))
		return false;
	for (size_t i = 0; i < offset.size(); i++)
	{
		b = *(DWORD*)b + offset[i];
		if (IsBadReadPtr((void*)b, 4))
			return false;
	}
	if (IsBadReadPtr((void*)b, sizeof(T)))
		return false;
	//if (IsBadWritePtr((void*)b, sizeof(T)))
	//{
	DWORD op;
	VirtualProtect((LPVOID)b, sizeof(T), PAGE_READWRITE, &op);
	*(T*)b = t;
	VirtualProtect((LPVOID)b, sizeof(T), op, &op);
	return true;
	//}
	*(T*)b = t;
	return true;
}

template <class T>
Address<T>& Address<T>::operator+(DWORD of)
{
	this->offset.push_back(of);
	return *this;
}
template <class T>
Address<T>::Address(DWORD bs)
{
	this->base = bs;
}