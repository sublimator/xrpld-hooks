#include <ripple/app/tx/applyHook.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/Slice.h>
using namespace ripple;


TER
hook::setHookState(
    beast::Journal& j,
    ApplyView& view,
    AccountID& account,
    Slice& data,
    Keylet const& accountKeylet,
    Keylet const& ownerDirKeylet,
    Keylet const& hookStateKeylet
    )
{

    auto const sle = view().peek(accountKeylet);
    if (!sle)
        return tefINTERNAL;

    // if the blob is too large don't set it
    if (data.size() > hook::state_max_blob_size) {
       return temHOOK_DATA_TOO_LARGE; 
    } 

    auto const oldHookState = view.peek(oldHookState);

    // if the blob is nil then delete the entry if it exists
    if (data.size() == 0) {
    
        if (!view.peek(hookStateKeylet))
            return tesSUCCESS; // a request to remove a non-existent entry is defined as success

        // Remove the node from the account directory.
        auto const hint = (*hookState)[sfOwnerNode];

        if (!view.dirRemove(ownerDirKeylet, hint, hookStateKeylet.key, false))
        {
            return tefBAD_LEDGER;
        }

        adjustOwnerCount(
            view,
            view.peek(accountKeylet),
            -1,
            j("View"));

        // remove the actual hook state obj
        view.erase(oldHookState);

        return tesSUCCESS;
    }

    // execution to this point means we are updating or creating a state hook
    
    auto const hookStateOld = view().peek(hookStateKeylet);
    if (hookStateOld) 
        view.erase(hookStateOld);

    // add new data to ledger
    auto hookStateNew = std::make_shared<SLE>(hookStateKeylet);
    view().insert(hookStateNew);
    hookStateNew->setFieldVL(sfHookData, data);


    if (!hookStateOld) {
        auto viewJ = j("View");
        // Add the hook to the account's directory if it wasn't there already
        auto const page = dirAdd(
            view,
            ownerDirKeylet,
            hookStateKeylet.key,
            false,
            describeOwnerDir(account),
            viewJ);
        
        JLOG(j.trace()) << "Create/update hook state for account " << toBase58(account)
                     << ": " << (page ? "success" : "failure");
        
        if (!page)
            return tecDIR_FULL;
    }

    //adjustOwnerCount(view(), sle, addedOwnerCount, viewJ);
    return tesSUCCESS;
}

void hook::print_wasmer_error()
{
  int error_len = wasmer_last_error_length();
  char *error_str = (char*)malloc(error_len);
  wasmer_last_error_message(error_str, error_len);
  printf("Error: `%s`\n", error_str);
    free(error_str);
}

TER hook::apply(Blob hook, ApplyContext& apply_ctx) {

    wasmer_instance_t *instance = NULL;


    if (wasmer_instantiate(&instance, hook.data(), hook.size(), imports, 1) != wasmer_result_t::WASMER_OK) {
        printf("hook malformed\n");
        print_wasmer_error();
        return temMALFORMED;
    }

    wasmer_instance_context_data_set ( instance, &apply_ctx );
        printf("Set ApplyContext: %lx\n", (void*)&apply_ctx);

    wasmer_value_t arguments[] = { { .tag = wasmer_value_tag::WASM_I64, .value = {.I64 = 0 } } };
    wasmer_value_t results[] = { { .tag = wasmer_value_tag::WASM_I64, .value = {.I64 = 0 } } };

    if (wasmer_instance_call(
        instance,
        "hook",
        arguments,
        1,
        results,
        1
    ) != wasmer_result_t::WASMER_OK) {
        printf("hook() call failed\n");
        print_wasmer_error();
        return temMALFORMED; /// todo: [RH] should be a hook execution error code tecHOOK_ERROR?
    }

    int64_t response_value = results[0].value.I64;

    printf("hook return code was: %ld\n", response_value);

    // todo: [RH] memory leak here, destroy the imports, instance using a smart pointer
    wasmer_instance_destroy(instance);
    printf("running hook 3\n");

    return tesSUCCESS;
}


int64_t hook_api::output_dbg ( wasmer_instance_context_t * wasm_ctx, uint32_t ptr, uint32_t len ) {

    HOOK_SETUP(); // populates memory_ctx, memory, memory_length, apply_ctx on current stack

    printf("HOOKAPI_output_dbg: ");
    if (len > 1024) len = 1024;
    for (int i = 0; i < len && i < memory_length; ++i)
        printf("%c", memory[ptr + i]);
    return len;

}
int64_t hook_api::set_state ( wasmer_instance_context_t * wasm_ctx, uint32_t key_ptr, uint32_t data_ptr_in, uint32_t in_len ) {

    HOOK_SETUP(); // populates memory_ctx, memory, memory_length, apply_ctx on current stack

    if (key_ptr + 32 > memory_length || data_ptr_in + hook::state_max_blob_size > memory_length) {
        JLOG(j.trace())
            << "Hook tried to set_state using memory outside of the wasm instance limit";
        return OUT_OF_BOUNDS;
    }

    

}

int64_t hook_api::get_state ( wasmer_instance_context_t * wasm_ctx, uint32_t ptr key_ptr, uint32_t data_ptr_out ) {

    HOOK_SETUP(); // populates memory_ctx, memory, memory_length, apply_ctx on current stack

}

/*int64_t hook_api::get_current_ledger_id ( wasmer_instance_context_t * wasm_ctx, uint32_t ptr ) {
    ripple::ApplyContext* apply_ctx = (ripple::ApplyContext*) wasmer_instance_context_data_get(wasm_ctx);
    uint8_t *memory = wasmer_memory_data( wasmer_instance_context_memory(wasm_ctx, 0) );
}*/