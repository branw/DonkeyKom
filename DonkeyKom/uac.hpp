#pragma once
#include <Windows.h>

bool can_be_elevated();

void elevate(LPWSTR cmd);
