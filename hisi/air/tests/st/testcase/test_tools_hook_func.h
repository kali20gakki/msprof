/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __INC_TEST_TOOLS_HOOK_FUNC_H
#define __INC_TEST_TOOLS_HOOK_FUNC_H

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cerrno>

#include <map>
#include <vector>
#include <string>

#include <dlfcn.h>
#include <cxxabi.h>
#include <sys/mman.h>
#include <sys/user.h>

//******************************************************************************
//  1. Hook normal c-like function
//  void TestFunc(uint32_t obj_id, void *obj, int32_t flags) {
//  }
//
//  void StubFunc(uint32_t obj_id, void *obj, int32_t flags) {
//  }
//
//  SetNormalHook(TestFunc, StubFunc);
//
//------------------------------------------------------------------------------
//  2. Hook static member function
//  class TestObject {
//   public:
//    static void TestFunc(uint32_t obj_id, void *obj, int32_t flags) {
//    }
//  };
//
//  void StubFunc(uint32_t obj_id, void *obj, int32_t flags) {
//  }
//
//  SetNormalHook(TestObject::TestFunc, StubFunc);
//------------------------------------------------------------------------------
//  3. Hook normal member function
//  class TestObject {
//   public:
//    void TestFunc(uint32_t obj_id, void *obj, int32_t flags) {
//    }
//  };
//
//  class StubObject {
//   public:
//    void StubFunc(uint32_t obj_id, void *obj, int32_t flags) {
//    }
//  };
//
//  SetNormalHook(&TestObject::TestFunc, &StubObject::StubFunc);
//------------------------------------------------------------------------------
//  4. Hook virtual member function
//  class TestObject {
//   public:
//    virtual void TestFunc(uint32_t obj_id, void *obj, int32_t flags) {
//    }
//  };
//
//  class StubObject {
//   public:
//    virtual void StubFunc(uint32_t obj_id, void *obj, int32_t flags) {
//    }
//  };
//
//  void *func = GetVirtualFunc<TestObject>("TestFunc", 0);
//  void *stub = GetVirtualFunc<StubObject>("StubFunc", 0);
//  SetNormalHook(func, stub);
//******************************************************************************
static std::map<uintptr_t *, std::vector<uint8_t>> hooked_func;

template<typename TEST_FUNC, typename STUB_FUNC>
void SetNormalHook(TEST_FUNC test_func, STUB_FUNC stub_func) {
  uintptr_t *test_vptr = *(reinterpret_cast<uintptr_t **>(&test_func));
  uintptr_t *stub_vptr = *(reinterpret_cast<uintptr_t **>(&stub_func));

  uintptr_t *func_page = reinterpret_cast<uintptr_t *>(reinterpret_cast<uintptr_t>(test_vptr) & PAGE_MASK);
  size_t func_size = (reinterpret_cast<uintptr_t>(test_vptr) - reinterpret_cast<uintptr_t>(func_page)) + sizeof(void *);
  if (mprotect(func_page, func_size, PROT_READ | PROT_EXEC | PROT_WRITE) != 0) {
    fprintf(stderr, "HOOK: mprotect[%p:%zu] failed[errno=%u]\n", func_page, func_size, errno);
    return;
  }

  if (hooked_func[test_vptr].empty()) {
    hooked_func[test_vptr].resize(12U);
    (void)memcpy(hooked_func[test_vptr].data(), test_vptr, 12U); // Save for Recover.
  }

  // GCC AT&T: Modify jump to stub function.
  uint8_t hook_code[] = {0x48, 0xb8, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, // movq $0x0, %rax
                         0xff, 0xe0}; // jmpq *%rax

  (void)memcpy(&hook_code[2], &stub_vptr, sizeof(void *)); // Set movq to stub function.
  (void)memcpy(test_vptr, hook_code, sizeof(hook_code));
  (void)mprotect(func_page, func_size, PROT_READ | PROT_EXEC);
}

template<typename TEST_FUNC>
void DelNormalHook(TEST_FUNC test_func) {
  uintptr_t *test_vptr = *(reinterpret_cast<uintptr_t **>(&test_func));
  const auto it = hooked_func.find(test_vptr);
  if (it == hooked_func.cend()) {
    fprintf(stderr, "HOOK: stub not found[errno=%u]\n", errno);
    return;
  }

  uintptr_t *func_page = reinterpret_cast<uintptr_t *>(reinterpret_cast<uintptr_t>(it->first) & PAGE_MASK);
  size_t func_size = reinterpret_cast<uintptr_t>(it->first) - reinterpret_cast<uintptr_t>(func_page) + sizeof(void *);
  if (mprotect(func_page, func_size, PROT_READ | PROT_EXEC | PROT_WRITE) != 0) {
    fprintf(stderr, "HOOK: mprotect[%p:%zu] failed[errno=%u]\n", func_page, func_size, errno);
    hooked_func.erase(it);
    return;
  }

  (void)memcpy(it->first, it->second.data(), it->second.size());
  (void)mprotect(func_page, func_size, PROT_READ | PROT_EXEC);
  hooked_func.erase(it);
}

void TearDownHooks() {
  for (const auto &item : hooked_func) {
    uintptr_t *func_page = reinterpret_cast<uintptr_t *>(reinterpret_cast<uintptr_t>(item.first) & PAGE_MASK);
    size_t func_size = reinterpret_cast<uintptr_t>(item.first) - reinterpret_cast<uintptr_t>(func_page) + sizeof(void *);
    if (mprotect(func_page, func_size, PROT_READ | PROT_EXEC | PROT_WRITE) != 0) {
      fprintf(stderr, "HOOK: mprotect[%p:%zu] failed[errno=%u]\n", func_page, func_size, errno);
      continue;
    }

    (void)memcpy(item.first, item.second.data(), item.second.size());
    (void)mprotect(func_page, func_size, PROT_READ | PROT_EXEC);
  }
  hooked_func.clear();
}

template<typename CXX_CLASS>
void *GetVirtualFunc(const char *name, uint32_t skip = 0U) {
  CXX_CLASS stub;
  const uintptr_t *vptr = *static_cast<uintptr_t **>(&stub);

  Dl_info dl_info;
  std::string str_class;
  uint32_t index = 0U;
  for (uint8_t i = 0U; i < 64U; ++i) {
    uintptr_t *func = reinterpret_cast<uintptr_t *>(vptr[i]);
    if (dladdr(func, &dl_info) == 0) {
      fprintf(stderr, "HOOK: GetVirtualFunc<%s> dladdr[%p] failed, errno:%u\n", name, func, errno);
      break;
    }

    const char *cxxabi = abi::__cxa_demangle(dl_info.dli_sname, 0, 0, 0);
    if (cxxabi == nullptr) {
      fprintf(stderr, "HOOK: GetVirtualFunc<%s> cxxabi[%s] failed, errno:%u\n", name, dl_info.dli_sname, errno);
      break;
    }

    const char *pos1 = strstr(cxxabi, "::");
    const char *pos2 = strstr(cxxabi, "(");
    if ((pos1 == nullptr) || (pos2 == nullptr)) {
      fprintf(stderr, "HOOK: GetVirtualFunc<%s> cxxabi[%s] invalid, errno:%u\n", name, cxxabi, errno);
      break;
    }

    const std::string info1(cxxabi, pos1 - cxxabi);
    const std::string info2(pos1 + 2, pos2 - (pos1 + 2));
    if (str_class.empty()) {
      str_class = info1;
    }

    if (str_class != info1) {
      break;
    }

    if (info2 == name) {
      if (index == skip) {
        return func;
      }
      ++index;
    }
  }

  return nullptr;
}
#endif  // __INC_TEST_TOOLS_HOOK_FUNC_H
