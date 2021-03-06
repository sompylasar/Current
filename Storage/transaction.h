/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2016 Maxim Zhurovich <zhurovich@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#ifndef CURRENT_STORAGE_TRANSACTION_H
#define CURRENT_STORAGE_TRANSACTION_H

#include "../port.h"
#include <chrono>

#include "../TypeSystem/struct.h"

namespace current {
namespace storage {

using TransactionMetaFields = std::map<std::string, std::string>;

CURRENT_STRUCT(TransactionMeta) {
  CURRENT_FIELD(begin_us, std::chrono::microseconds);
  CURRENT_FIELD(end_us, std::chrono::microseconds);
  CURRENT_FIELD(fields, TransactionMetaFields);
  CURRENT_DEFAULT_CONSTRUCTOR(TransactionMeta) : begin_us(0), end_us(0) {}
};

CURRENT_STRUCT_T(Transaction) {
  using variant_t = T;
  CURRENT_FIELD(meta, TransactionMeta);
  CURRENT_FIELD(mutations, std::vector<T>);
};

}  // namespace storage
}  // namespace current

#endif  // CURRENT_STORAGE_TRANSACTION_H
