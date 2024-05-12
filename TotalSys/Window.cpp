#include "pch.h"
#include "Window.h"

bool Window::IsOpen() const {
	return m_Open;
}

void Window::Open(bool open) {
	m_Open = open;
}

bool* Window::GetOpenAddress() {
	return &m_Open;
}
