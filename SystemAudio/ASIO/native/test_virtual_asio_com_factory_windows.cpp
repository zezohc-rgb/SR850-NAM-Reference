#include "virtual_asio_com_factory_windows.hpp"
#include <cassert>
#ifdef _WIN32
int main() {
  auto* factory=new sr850::VirtualAsioClassFactory;
  void* ptr=reinterpret_cast<void*>(1);
  assert(factory->QueryInterface(IID_IClassFactory,&ptr)==S_OK && ptr);
  static_cast<IClassFactory*>(ptr)->Release();
  ptr=reinterpret_cast<void*>(1);
  assert(factory->CreateInstance(nullptr,IID_IUnknown,&ptr)==E_NOINTERFACE);
  assert(ptr==nullptr);
  assert(factory->CreateInstance(nullptr,IID_IUnknown,nullptr)==E_POINTER);
  assert(factory->LockServer(TRUE)==S_OK);
  assert(sr850::VirtualAsioClassFactory::server_locks()==1);
  assert(factory->LockServer(FALSE)==S_OK);
  assert(sr850::VirtualAsioClassFactory::server_locks()==0);
  assert(factory->Release()==0);
}
#else
int main(){return 0;}
#endif
