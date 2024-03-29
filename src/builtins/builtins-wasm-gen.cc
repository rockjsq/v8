// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-utils-gen.h"
#include "src/codegen/code-stub-assembler.h"
#include "src/codegen/interface-descriptors.h"
#include "src/objects/objects-inl.h"
#include "src/wasm/wasm-objects.h"
#include "src/wasm/wasm-opcodes.h"

namespace v8 {
namespace internal {

class WasmBuiltinsAssembler : public CodeStubAssembler {
 public:
  explicit WasmBuiltinsAssembler(compiler::CodeAssemblerState* state)
      : CodeStubAssembler(state) {}

 protected:
  TNode<Object> UncheckedParameter(int index) {
    return UncheckedCast<Object>(Parameter(index));
  }

  TNode<Object> LoadInstanceFromFrame() {
    return UncheckedCast<Object>(
        LoadFromParentFrame(WasmCompiledFrameConstants::kWasmInstanceOffset));
  }

  TNode<Object> LoadContextFromInstance(TNode<Object> instance) {
    return UncheckedCast<Object>(
        Load(MachineType::AnyTagged(), instance,
             IntPtrConstant(WasmInstanceObject::kNativeContextOffset -
                            kHeapObjectTag)));
  }
};

TF_BUILTIN(WasmStackGuard, WasmBuiltinsAssembler) {
  TNode<Object> instance = LoadInstanceFromFrame();
  TNode<Object> context = LoadContextFromInstance(instance);
  TailCallRuntime(Runtime::kWasmStackGuard, context);
}

TF_BUILTIN(WasmStackOverflow, WasmBuiltinsAssembler) {
  TNode<Object> instance = LoadInstanceFromFrame();
  TNode<Object> context = LoadContextFromInstance(instance);
  TailCallRuntime(Runtime::kThrowWasmStackOverflow, context);
}

TF_BUILTIN(WasmThrow, WasmBuiltinsAssembler) {
  TNode<Object> exception = UncheckedParameter(Descriptor::kException);
  TNode<Object> instance = LoadInstanceFromFrame();
  TNode<Object> context = LoadContextFromInstance(instance);
  TailCallRuntime(Runtime::kThrow, context, exception);
}

TF_BUILTIN(WasmRethrow, WasmBuiltinsAssembler) {
  TNode<Object> exception = UncheckedParameter(Descriptor::kException);
  TNode<Object> instance = LoadInstanceFromFrame();
  TNode<Object> context = LoadContextFromInstance(instance);
  TailCallRuntime(Runtime::kReThrow, context, exception);
}

TF_BUILTIN(WasmAtomicNotify, WasmBuiltinsAssembler) {
  TNode<Uint32T> address =
      UncheckedCast<Uint32T>(Parameter(Descriptor::kAddress));
  TNode<Uint32T> count = UncheckedCast<Uint32T>(Parameter(Descriptor::kCount));

  TNode<Object> instance = LoadInstanceFromFrame();
  TNode<Number> address_number = ChangeUint32ToTagged(address);
  TNode<Number> count_number = ChangeUint32ToTagged(count);

  TNode<Smi> result_smi = UncheckedCast<Smi>(
      CallRuntime(Runtime::kWasmAtomicNotify, NoContextConstant(), instance,
                  address_number, count_number));
  ReturnRaw(SmiToInt32(result_smi));
}

TF_BUILTIN(WasmI32AtomicWait, WasmBuiltinsAssembler) {
  TNode<Uint32T> address =
      UncheckedCast<Uint32T>(Parameter(Descriptor::kAddress));
  TNode<Int32T> expected_value =
      UncheckedCast<Int32T>(Parameter(Descriptor::kExpectedValue));
  TNode<Float64T> timeout =
      UncheckedCast<Float64T>(Parameter(Descriptor::kTimeout));

  TNode<Object> instance = LoadInstanceFromFrame();
  TNode<Number> address_number = ChangeUint32ToTagged(address);
  TNode<Number> expected_value_number = ChangeInt32ToTagged(expected_value);
  TNode<Number> timeout_number = ChangeFloat64ToTagged(timeout);

  TNode<Smi> result_smi = UncheckedCast<Smi>(
      CallRuntime(Runtime::kWasmI32AtomicWait, NoContextConstant(), instance,
                  address_number, expected_value_number, timeout_number));
  ReturnRaw(SmiToInt32(result_smi));
}

TF_BUILTIN(WasmI64AtomicWait, WasmBuiltinsAssembler) {
  TNode<Uint32T> address =
      UncheckedCast<Uint32T>(Parameter(Descriptor::kAddress));
  TNode<Uint32T> expected_value_high =
      UncheckedCast<Uint32T>(Parameter(Descriptor::kExpectedValueHigh));
  TNode<Uint32T> expected_value_low =
      UncheckedCast<Uint32T>(Parameter(Descriptor::kExpectedValueLow));
  TNode<Float64T> timeout =
      UncheckedCast<Float64T>(Parameter(Descriptor::kTimeout));

  TNode<Object> instance = LoadInstanceFromFrame();
  TNode<Number> address_number = ChangeUint32ToTagged(address);
  TNode<Number> expected_value_high_number =
      ChangeUint32ToTagged(expected_value_high);
  TNode<Number> expected_value_low_number =
      ChangeUint32ToTagged(expected_value_low);
  TNode<Number> timeout_number = ChangeFloat64ToTagged(timeout);

  TNode<Smi> result_smi = UncheckedCast<Smi>(
      CallRuntime(Runtime::kWasmI64AtomicWait, NoContextConstant(), instance,
                  address_number, expected_value_high_number,
                  expected_value_low_number, timeout_number));
  ReturnRaw(SmiToInt32(result_smi));
}

TF_BUILTIN(WasmMemoryGrow, WasmBuiltinsAssembler) {
  TNode<Int32T> num_pages =
      UncheckedCast<Int32T>(Parameter(Descriptor::kNumPages));
  Label num_pages_out_of_range(this, Label::kDeferred);

  TNode<BoolT> num_pages_fits_in_smi =
      IsValidPositiveSmi(ChangeInt32ToIntPtr(num_pages));
  GotoIfNot(num_pages_fits_in_smi, &num_pages_out_of_range);

  TNode<Smi> num_pages_smi = SmiFromInt32(num_pages);
  TNode<Object> instance = LoadInstanceFromFrame();
  TNode<Object> context = LoadContextFromInstance(instance);
  TNode<Smi> ret_smi = UncheckedCast<Smi>(
      CallRuntime(Runtime::kWasmMemoryGrow, context, instance, num_pages_smi));
  TNode<Int32T> ret = SmiToInt32(ret_smi);
  ReturnRaw(ret);

  BIND(&num_pages_out_of_range);
  ReturnRaw(Int32Constant(-1));
}

TF_BUILTIN(WasmTableGet, WasmBuiltinsAssembler) {
  TNode<Int32T> entry_index =
      UncheckedCast<Int32T>(Parameter(Descriptor::kEntryIndex));
  TNode<Object> instance = LoadInstanceFromFrame();
  TNode<Object> context = LoadContextFromInstance(instance);
  Label entry_index_out_of_range(this, Label::kDeferred);

  TNode<BoolT> entry_index_fits_in_smi =
      IsValidPositiveSmi(ChangeInt32ToIntPtr(entry_index));
  GotoIfNot(entry_index_fits_in_smi, &entry_index_out_of_range);

  TNode<Smi> entry_index_smi = SmiFromInt32(entry_index);
  TNode<Smi> table_index_smi =
      UncheckedCast<Smi>(Parameter(Descriptor::kTableIndex));

  TailCallRuntime(Runtime::kWasmFunctionTableGet, context, instance,
                  table_index_smi, entry_index_smi);

  BIND(&entry_index_out_of_range);
  MessageTemplate message_id =
      wasm::WasmOpcodes::TrapReasonToMessageId(wasm::kTrapTableOutOfBounds);
  TailCallRuntime(Runtime::kThrowWasmError, context,
                  SmiConstant(static_cast<int>(message_id)));
}

TF_BUILTIN(WasmTableSet, WasmBuiltinsAssembler) {
  TNode<Int32T> entry_index =
      UncheckedCast<Int32T>(Parameter(Descriptor::kEntryIndex));
  TNode<Object> instance = LoadInstanceFromFrame();
  TNode<Object> context = LoadContextFromInstance(instance);
  Label entry_index_out_of_range(this, Label::kDeferred);

  TNode<BoolT> entry_index_fits_in_smi =
      IsValidPositiveSmi(ChangeInt32ToIntPtr(entry_index));
  GotoIfNot(entry_index_fits_in_smi, &entry_index_out_of_range);

  TNode<Smi> entry_index_smi = SmiFromInt32(entry_index);
  TNode<Smi> table_index_smi =
      UncheckedCast<Smi>(Parameter(Descriptor::kTableIndex));
  TNode<Object> value = UncheckedCast<Object>(Parameter(Descriptor::kValue));
  TailCallRuntime(Runtime::kWasmFunctionTableSet, context, instance,
                  table_index_smi, entry_index_smi, value);

  BIND(&entry_index_out_of_range);
  MessageTemplate message_id =
      wasm::WasmOpcodes::TrapReasonToMessageId(wasm::kTrapTableOutOfBounds);
  TailCallRuntime(Runtime::kThrowWasmError, context,
                  SmiConstant(static_cast<int>(message_id)));
}

#define DECLARE_ENUM(name)                                       \
  TF_BUILTIN(ThrowWasm##name, WasmBuiltinsAssembler) {           \
    TNode<Object> instance = LoadInstanceFromFrame();            \
    TNode<Object> context = LoadContextFromInstance(instance);   \
    MessageTemplate message_id =                                 \
        wasm::WasmOpcodes::TrapReasonToMessageId(wasm::k##name); \
    TailCallRuntime(Runtime::kThrowWasmError, context,           \
                    SmiConstant(static_cast<int>(message_id)));  \
  }
FOREACH_WASM_TRAPREASON(DECLARE_ENUM)
#undef DECLARE_ENUM

}  // namespace internal
}  // namespace v8
