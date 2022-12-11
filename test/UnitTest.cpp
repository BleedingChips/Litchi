#include "Litchi/LitchiHttpConnect.h"

using namespace Litchi;

int main()
{
	auto Http = HttpConnection::Create(u8"www.baidu.com", 80);
	return 0;
}