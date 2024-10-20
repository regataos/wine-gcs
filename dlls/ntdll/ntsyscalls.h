/* Automatically generated by tools/make_specfiles */

#define ALL_SYSCALLS32 \
    SYSCALL_ENTRY( 0x0000, NtAcceptConnectPort, 24 ) \
    SYSCALL_ENTRY( 0x0001, NtAccessCheck, 32 ) \
    SYSCALL_ENTRY( 0x0002, NtAccessCheckAndAuditAlarm, 44 ) \
    SYSCALL_ENTRY( 0x0003, NtAddAtom, 12 ) \
    SYSCALL_ENTRY( 0x0004, NtAdjustGroupsToken, 24 ) \
    SYSCALL_ENTRY( 0x0005, NtAdjustPrivilegesToken, 24 ) \
    SYSCALL_ENTRY( 0x0006, NtAlertResumeThread, 8 ) \
    SYSCALL_ENTRY( 0x0007, NtAlertThread, 4 ) \
    SYSCALL_ENTRY( 0x0008, NtAlertThreadByThreadId, 4 ) \
    SYSCALL_ENTRY( 0x0009, NtAllocateLocallyUniqueId, 4 ) \
    SYSCALL_ENTRY( 0x000a, NtAllocateUuids, 16 ) \
    SYSCALL_ENTRY( 0x000b, NtAllocateVirtualMemory, 24 ) \
    SYSCALL_ENTRY( 0x000c, NtAllocateVirtualMemoryEx, 28 ) \
    SYSCALL_ENTRY( 0x000d, NtAreMappedFilesTheSame, 8 ) \
    SYSCALL_ENTRY( 0x000e, NtAssignProcessToJobObject, 8 ) \
    SYSCALL_ENTRY( 0x000f, NtCallbackReturn, 12 ) \
    SYSCALL_ENTRY( 0x0010, NtCancelIoFile, 8 ) \
    SYSCALL_ENTRY( 0x0011, NtCancelIoFileEx, 12 ) \
    SYSCALL_ENTRY( 0x0012, NtCancelSynchronousIoFile, 12 ) \
    SYSCALL_ENTRY( 0x0013, NtCancelTimer, 8 ) \
    SYSCALL_ENTRY( 0x0014, NtClearEvent, 4 ) \
    SYSCALL_ENTRY( 0x0015, NtClose, 4 ) \
    SYSCALL_ENTRY( 0x0016, NtCommitTransaction, 8 ) \
    SYSCALL_ENTRY( 0x0017, NtCompareObjects, 8 ) \
    SYSCALL_ENTRY( 0x0018, NtCompleteConnectPort, 4 ) \
    SYSCALL_ENTRY( 0x0019, NtConnectPort, 32 ) \
    SYSCALL_ENTRY( 0x001a, NtContinue, 8 ) \
    SYSCALL_ENTRY( 0x001b, NtCreateDebugObject, 16 ) \
    SYSCALL_ENTRY( 0x001c, NtCreateDirectoryObject, 12 ) \
    SYSCALL_ENTRY( 0x001d, NtCreateEvent, 20 ) \
    SYSCALL_ENTRY( 0x001e, NtCreateFile, 44 ) \
    SYSCALL_ENTRY( 0x001f, NtCreateIoCompletion, 16 ) \
    SYSCALL_ENTRY( 0x0020, NtCreateJobObject, 12 ) \
    SYSCALL_ENTRY( 0x0021, NtCreateKey, 28 ) \
    SYSCALL_ENTRY( 0x0022, NtCreateKeyTransacted, 32 ) \
    SYSCALL_ENTRY( 0x0023, NtCreateKeyedEvent, 16 ) \
    SYSCALL_ENTRY( 0x0024, NtCreateLowBoxToken, 36 ) \
    SYSCALL_ENTRY( 0x0025, NtCreateMailslotFile, 32 ) \
    SYSCALL_ENTRY( 0x0026, NtCreateMutant, 16 ) \
    SYSCALL_ENTRY( 0x0027, NtCreateNamedPipeFile, 56 ) \
    SYSCALL_ENTRY( 0x0028, NtCreatePagingFile, 16 ) \
    SYSCALL_ENTRY( 0x0029, NtCreatePort, 20 ) \
    SYSCALL_ENTRY( 0x002a, NtCreateSection, 28 ) \
    SYSCALL_ENTRY( 0x002b, NtCreateSemaphore, 20 ) \
    SYSCALL_ENTRY( 0x002c, NtCreateSymbolicLinkObject, 16 ) \
    SYSCALL_ENTRY( 0x002d, NtCreateThread, 32 ) \
    SYSCALL_ENTRY( 0x002e, NtCreateThreadEx, 44 ) \
    SYSCALL_ENTRY( 0x002f, NtCreateTimer, 16 ) \
    SYSCALL_ENTRY( 0x0030, NtCreateToken, 52 ) \
    SYSCALL_ENTRY( 0x0031, NtCreateTransaction, 40 ) \
    SYSCALL_ENTRY( 0x0032, NtCreateUserProcess, 44 ) \
    SYSCALL_ENTRY( 0x0033, NtDebugActiveProcess, 8 ) \
    SYSCALL_ENTRY( 0x0034, NtDebugContinue, 12 ) \
    SYSCALL_ENTRY( 0x0035, NtDelayExecution, 8 ) \
    SYSCALL_ENTRY( 0x0036, NtDeleteAtom, 4 ) \
    SYSCALL_ENTRY( 0x0037, NtDeleteFile, 4 ) \
    SYSCALL_ENTRY( 0x0038, NtDeleteKey, 4 ) \
    SYSCALL_ENTRY( 0x0039, NtDeleteValueKey, 8 ) \
    SYSCALL_ENTRY( 0x003a, NtDeviceIoControlFile, 40 ) \
    SYSCALL_ENTRY( 0x003b, NtDisplayString, 4 ) \
    SYSCALL_ENTRY( 0x003c, NtDuplicateObject, 28 ) \
    SYSCALL_ENTRY( 0x003d, NtDuplicateToken, 24 ) \
    SYSCALL_ENTRY( 0x003e, NtEnumerateKey, 24 ) \
    SYSCALL_ENTRY( 0x003f, NtEnumerateValueKey, 24 ) \
    SYSCALL_ENTRY( 0x0040, NtFilterToken, 24 ) \
    SYSCALL_ENTRY( 0x0041, NtFindAtom, 12 ) \
    SYSCALL_ENTRY( 0x0042, NtFlushBuffersFile, 8 ) \
    SYSCALL_ENTRY( 0x0043, NtFlushInstructionCache, 12 ) \
    SYSCALL_ENTRY( 0x0044, NtFlushKey, 4 ) \
    SYSCALL_ENTRY( 0x0045, NtFlushProcessWriteBuffers, 0 ) \
    SYSCALL_ENTRY( 0x0046, NtFlushVirtualMemory, 16 ) \
    SYSCALL_ENTRY( 0x0047, NtFreeVirtualMemory, 16 ) \
    SYSCALL_ENTRY( 0x0048, NtFsControlFile, 40 ) \
    SYSCALL_ENTRY( 0x0049, NtGetContextThread, 8 ) \
    SYSCALL_ENTRY( 0x004a, NtGetCurrentProcessorNumber, 0 ) \
    SYSCALL_ENTRY( 0x004b, NtGetNextThread, 24 ) \
    SYSCALL_ENTRY( 0x004c, NtGetNlsSectionPtr, 20 ) \
    SYSCALL_ENTRY( 0x004d, NtGetWriteWatch, 28 ) \
    SYSCALL_ENTRY( 0x004e, NtImpersonateAnonymousToken, 4 ) \
    SYSCALL_ENTRY( 0x004f, NtInitializeNlsFiles, 12 ) \
    SYSCALL_ENTRY( 0x0050, NtInitiatePowerAction, 16 ) \
    SYSCALL_ENTRY( 0x0051, NtIsProcessInJob, 8 ) \
    SYSCALL_ENTRY( 0x0052, NtListenPort, 8 ) \
    SYSCALL_ENTRY( 0x0053, NtLoadDriver, 4 ) \
    SYSCALL_ENTRY( 0x0054, NtLoadKey, 8 ) \
    SYSCALL_ENTRY( 0x0055, NtLoadKey2, 12 ) \
    SYSCALL_ENTRY( 0x0056, NtLoadKeyEx, 32 ) \
    SYSCALL_ENTRY( 0x0057, NtLockFile, 40 ) \
    SYSCALL_ENTRY( 0x0058, NtLockVirtualMemory, 16 ) \
    SYSCALL_ENTRY( 0x0059, NtMakeTemporaryObject, 4 ) \
    SYSCALL_ENTRY( 0x005a, NtMapViewOfSection, 40 ) \
    SYSCALL_ENTRY( 0x005b, NtMapViewOfSectionEx, 36 ) \
    SYSCALL_ENTRY( 0x005c, NtNotifyChangeDirectoryFile, 36 ) \
    SYSCALL_ENTRY( 0x005d, NtNotifyChangeKey, 40 ) \
    SYSCALL_ENTRY( 0x005e, NtNotifyChangeMultipleKeys, 48 ) \
    SYSCALL_ENTRY( 0x005f, NtOpenDirectoryObject, 12 ) \
    SYSCALL_ENTRY( 0x0060, NtOpenEvent, 12 ) \
    SYSCALL_ENTRY( 0x0061, NtOpenFile, 24 ) \
    SYSCALL_ENTRY( 0x0062, NtOpenIoCompletion, 12 ) \
    SYSCALL_ENTRY( 0x0063, NtOpenJobObject, 12 ) \
    SYSCALL_ENTRY( 0x0064, NtOpenKey, 12 ) \
    SYSCALL_ENTRY( 0x0065, NtOpenKeyEx, 16 ) \
    SYSCALL_ENTRY( 0x0066, NtOpenKeyTransacted, 16 ) \
    SYSCALL_ENTRY( 0x0067, NtOpenKeyTransactedEx, 20 ) \
    SYSCALL_ENTRY( 0x0068, NtOpenKeyedEvent, 12 ) \
    SYSCALL_ENTRY( 0x0069, NtOpenMutant, 12 ) \
    SYSCALL_ENTRY( 0x006a, NtOpenProcess, 16 ) \
    SYSCALL_ENTRY( 0x006b, NtOpenProcessToken, 12 ) \
    SYSCALL_ENTRY( 0x006c, NtOpenProcessTokenEx, 16 ) \
    SYSCALL_ENTRY( 0x006d, NtOpenSection, 12 ) \
    SYSCALL_ENTRY( 0x006e, NtOpenSemaphore, 12 ) \
    SYSCALL_ENTRY( 0x006f, NtOpenSymbolicLinkObject, 12 ) \
    SYSCALL_ENTRY( 0x0070, NtOpenThread, 16 ) \
    SYSCALL_ENTRY( 0x0071, NtOpenThreadToken, 16 ) \
    SYSCALL_ENTRY( 0x0072, NtOpenThreadTokenEx, 20 ) \
    SYSCALL_ENTRY( 0x0073, NtOpenTimer, 12 ) \
    SYSCALL_ENTRY( 0x0074, NtPowerInformation, 20 ) \
    SYSCALL_ENTRY( 0x0075, NtPrivilegeCheck, 12 ) \
    SYSCALL_ENTRY( 0x0076, NtProtectVirtualMemory, 20 ) \
    SYSCALL_ENTRY( 0x0077, NtPulseEvent, 8 ) \
    SYSCALL_ENTRY( 0x0078, NtQueryAttributesFile, 8 ) \
    SYSCALL_ENTRY( 0x0079, NtQueryDefaultLocale, 8 ) \
    SYSCALL_ENTRY( 0x007a, NtQueryDefaultUILanguage, 4 ) \
    SYSCALL_ENTRY( 0x007b, NtQueryDirectoryFile, 44 ) \
    SYSCALL_ENTRY( 0x007c, NtQueryDirectoryObject, 28 ) \
    SYSCALL_ENTRY( 0x007d, NtQueryEaFile, 36 ) \
    SYSCALL_ENTRY( 0x007e, NtQueryEvent, 20 ) \
    SYSCALL_ENTRY( 0x007f, NtQueryFullAttributesFile, 8 ) \
    SYSCALL_ENTRY( 0x0080, NtQueryInformationAtom, 20 ) \
    SYSCALL_ENTRY( 0x0081, NtQueryInformationFile, 20 ) \
    SYSCALL_ENTRY( 0x0082, NtQueryInformationJobObject, 20 ) \
    SYSCALL_ENTRY( 0x0083, NtQueryInformationProcess, 20 ) \
    SYSCALL_ENTRY( 0x0084, NtQueryInformationThread, 20 ) \
    SYSCALL_ENTRY( 0x0085, NtQueryInformationToken, 20 ) \
    SYSCALL_ENTRY( 0x0086, NtQueryInstallUILanguage, 4 ) \
    SYSCALL_ENTRY( 0x0087, NtQueryIoCompletion, 20 ) \
    SYSCALL_ENTRY( 0x0088, NtQueryKey, 20 ) \
    SYSCALL_ENTRY( 0x0089, NtQueryLicenseValue, 20 ) \
    SYSCALL_ENTRY( 0x008a, NtQueryMultipleValueKey, 24 ) \
    SYSCALL_ENTRY( 0x008b, NtQueryMutant, 20 ) \
    SYSCALL_ENTRY( 0x008c, NtQueryObject, 20 ) \
    SYSCALL_ENTRY( 0x008d, NtQueryPerformanceCounter, 8 ) \
    SYSCALL_ENTRY( 0x008e, NtQuerySection, 20 ) \
    SYSCALL_ENTRY( 0x008f, NtQuerySecurityObject, 20 ) \
    SYSCALL_ENTRY( 0x0090, NtQuerySemaphore, 20 ) \
    SYSCALL_ENTRY( 0x0091, NtQuerySymbolicLinkObject, 12 ) \
    SYSCALL_ENTRY( 0x0092, NtQuerySystemEnvironmentValue, 16 ) \
    SYSCALL_ENTRY( 0x0093, NtQuerySystemEnvironmentValueEx, 20 ) \
    SYSCALL_ENTRY( 0x0094, NtQuerySystemInformation, 16 ) \
    SYSCALL_ENTRY( 0x0095, NtQuerySystemInformationEx, 24 ) \
    SYSCALL_ENTRY( 0x0096, NtQuerySystemTime, 4 ) \
    SYSCALL_ENTRY( 0x0097, NtQueryTimer, 20 ) \
    SYSCALL_ENTRY( 0x0098, NtQueryTimerResolution, 12 ) \
    SYSCALL_ENTRY( 0x0099, NtQueryValueKey, 24 ) \
    SYSCALL_ENTRY( 0x009a, NtQueryVirtualMemory, 24 ) \
    SYSCALL_ENTRY( 0x009b, NtQueryVolumeInformationFile, 20 ) \
    SYSCALL_ENTRY( 0x009c, NtQueueApcThread, 20 ) \
    SYSCALL_ENTRY( 0x009d, NtRaiseException, 12 ) \
    SYSCALL_ENTRY( 0x009e, NtRaiseHardError, 24 ) \
    SYSCALL_ENTRY( 0x009f, NtReadFile, 36 ) \
    SYSCALL_ENTRY( 0x00a0, NtReadFileScatter, 36 ) \
    SYSCALL_ENTRY( 0x00a1, NtReadVirtualMemory, 20 ) \
    SYSCALL_ENTRY( 0x00a2, NtRegisterThreadTerminatePort, 4 ) \
    SYSCALL_ENTRY( 0x00a3, NtReleaseKeyedEvent, 16 ) \
    SYSCALL_ENTRY( 0x00a4, NtReleaseMutant, 8 ) \
    SYSCALL_ENTRY( 0x00a5, NtReleaseSemaphore, 12 ) \
    SYSCALL_ENTRY( 0x00a6, NtRemoveIoCompletion, 20 ) \
    SYSCALL_ENTRY( 0x00a7, NtRemoveIoCompletionEx, 24 ) \
    SYSCALL_ENTRY( 0x00a8, NtRemoveProcessDebug, 8 ) \
    SYSCALL_ENTRY( 0x00a9, NtRenameKey, 8 ) \
    SYSCALL_ENTRY( 0x00aa, NtReplaceKey, 12 ) \
    SYSCALL_ENTRY( 0x00ab, NtReplyWaitReceivePort, 16 ) \
    SYSCALL_ENTRY( 0x00ac, NtRequestWaitReplyPort, 12 ) \
    SYSCALL_ENTRY( 0x00ad, NtResetEvent, 8 ) \
    SYSCALL_ENTRY( 0x00ae, NtResetWriteWatch, 12 ) \
    SYSCALL_ENTRY( 0x00af, NtRestoreKey, 12 ) \
    SYSCALL_ENTRY( 0x00b0, NtResumeProcess, 4 ) \
    SYSCALL_ENTRY( 0x00b1, NtResumeThread, 8 ) \
    SYSCALL_ENTRY( 0x00b2, NtRollbackTransaction, 8 ) \
    SYSCALL_ENTRY( 0x00b3, NtSaveKey, 8 ) \
    SYSCALL_ENTRY( 0x00b4, NtSecureConnectPort, 36 ) \
    SYSCALL_ENTRY( 0x00b5, NtSetContextThread, 8 ) \
    SYSCALL_ENTRY( 0x00b6, NtSetDebugFilterState, 12 ) \
    SYSCALL_ENTRY( 0x00b7, NtSetDefaultLocale, 8 ) \
    SYSCALL_ENTRY( 0x00b8, NtSetDefaultUILanguage, 4 ) \
    SYSCALL_ENTRY( 0x00b9, NtSetEaFile, 16 ) \
    SYSCALL_ENTRY( 0x00ba, NtSetEvent, 8 ) \
    SYSCALL_ENTRY( 0x00bb, NtSetInformationDebugObject, 20 ) \
    SYSCALL_ENTRY( 0x00bc, NtSetInformationFile, 20 ) \
    SYSCALL_ENTRY( 0x00bd, NtSetInformationJobObject, 16 ) \
    SYSCALL_ENTRY( 0x00be, NtSetInformationKey, 16 ) \
    SYSCALL_ENTRY( 0x00bf, NtSetInformationObject, 16 ) \
    SYSCALL_ENTRY( 0x00c0, NtSetInformationProcess, 16 ) \
    SYSCALL_ENTRY( 0x00c1, NtSetInformationThread, 16 ) \
    SYSCALL_ENTRY( 0x00c2, NtSetInformationToken, 16 ) \
    SYSCALL_ENTRY( 0x00c3, NtSetInformationVirtualMemory, 24 ) \
    SYSCALL_ENTRY( 0x00c4, NtSetIntervalProfile, 8 ) \
    SYSCALL_ENTRY( 0x00c5, NtSetIoCompletion, 20 ) \
    SYSCALL_ENTRY( 0x00c6, NtSetLdtEntries, 24 ) \
    SYSCALL_ENTRY( 0x00c7, NtSetSecurityObject, 12 ) \
    SYSCALL_ENTRY( 0x00c8, NtSetSystemInformation, 12 ) \
    SYSCALL_ENTRY( 0x00c9, NtSetSystemTime, 8 ) \
    SYSCALL_ENTRY( 0x00ca, NtSetThreadExecutionState, 8 ) \
    SYSCALL_ENTRY( 0x00cb, NtSetTimer, 28 ) \
    SYSCALL_ENTRY( 0x00cc, NtSetTimerResolution, 12 ) \
    SYSCALL_ENTRY( 0x00cd, NtSetValueKey, 24 ) \
    SYSCALL_ENTRY( 0x00ce, NtSetVolumeInformationFile, 20 ) \
    SYSCALL_ENTRY( 0x00cf, NtShutdownSystem, 4 ) \
    SYSCALL_ENTRY( 0x00d0, NtSignalAndWaitForSingleObject, 16 ) \
    SYSCALL_ENTRY( 0x00d1, NtSuspendProcess, 4 ) \
    SYSCALL_ENTRY( 0x00d2, NtSuspendThread, 8 ) \
    SYSCALL_ENTRY( 0x00d3, NtSystemDebugControl, 24 ) \
    SYSCALL_ENTRY( 0x00d4, NtTerminateJobObject, 8 ) \
    SYSCALL_ENTRY( 0x00d5, NtTerminateProcess, 8 ) \
    SYSCALL_ENTRY( 0x00d6, NtTerminateThread, 8 ) \
    SYSCALL_ENTRY( 0x00d7, NtTestAlert, 0 ) \
    SYSCALL_ENTRY( 0x00d8, NtTraceControl, 24 ) \
    SYSCALL_ENTRY( 0x00d9, NtUnloadDriver, 4 ) \
    SYSCALL_ENTRY( 0x00da, NtUnloadKey, 4 ) \
    SYSCALL_ENTRY( 0x00db, NtUnlockFile, 20 ) \
    SYSCALL_ENTRY( 0x00dc, NtUnlockVirtualMemory, 16 ) \
    SYSCALL_ENTRY( 0x00dd, NtUnmapViewOfSection, 8 ) \
    SYSCALL_ENTRY( 0x00de, NtUnmapViewOfSectionEx, 12 ) \
    SYSCALL_ENTRY( 0x00df, NtWaitForAlertByThreadId, 8 ) \
    SYSCALL_ENTRY( 0x00e0, NtWaitForDebugEvent, 16 ) \
    SYSCALL_ENTRY( 0x00e1, NtWaitForKeyedEvent, 16 ) \
    SYSCALL_ENTRY( 0x00e2, NtWaitForMultipleObjects, 20 ) \
    SYSCALL_ENTRY( 0x00e3, NtWaitForSingleObject, 12 ) \
    SYSCALL_ENTRY( 0x00e4, NtWow64AllocateVirtualMemory64, 28 ) \
    SYSCALL_ENTRY( 0x00e5, NtWow64GetNativeSystemInformation, 16 ) \
    SYSCALL_ENTRY( 0x00e6, NtWow64IsProcessorFeaturePresent, 4 ) \
    SYSCALL_ENTRY( 0x00e7, NtWow64ReadVirtualMemory64, 28 ) \
    SYSCALL_ENTRY( 0x00e8, NtWow64WriteVirtualMemory64, 28 ) \
    SYSCALL_ENTRY( 0x00e9, NtWriteFile, 36 ) \
    SYSCALL_ENTRY( 0x00ea, NtWriteFileGather, 36 ) \
    SYSCALL_ENTRY( 0x00eb, NtWriteVirtualMemory, 20 ) \
    SYSCALL_ENTRY( 0x00ec, NtYieldExecution, 0 ) \
    SYSCALL_ENTRY( 0x00ed, __wine_dbg_ftrace, 12 ) \
    SYSCALL_ENTRY( 0x00ee, __wine_needs_override_large_address_aware, 0 ) \
    SYSCALL_ENTRY( 0x00ef, __wine_set_unix_env, 8 ) \
    SYSCALL_ENTRY( 0x00f0, wine_nt_to_unix_file_name, 16 ) \
    SYSCALL_ENTRY( 0x00f1, wine_unix_to_nt_file_name, 12 )

#define ALL_SYSCALLS64 \
    SYSCALL_ENTRY( 0x0000, NtAcceptConnectPort, 48 ) \
    SYSCALL_ENTRY( 0x0001, NtAccessCheck, 64 ) \
    SYSCALL_ENTRY( 0x0002, NtAccessCheckAndAuditAlarm, 88 ) \
    SYSCALL_ENTRY( 0x0003, NtAddAtom, 24 ) \
    SYSCALL_ENTRY( 0x0004, NtAdjustGroupsToken, 48 ) \
    SYSCALL_ENTRY( 0x0005, NtAdjustPrivilegesToken, 48 ) \
    SYSCALL_ENTRY( 0x0006, NtAlertResumeThread, 16 ) \
    SYSCALL_ENTRY( 0x0007, NtAlertThread, 8 ) \
    SYSCALL_ENTRY( 0x0008, NtAlertThreadByThreadId, 8 ) \
    SYSCALL_ENTRY( 0x0009, NtAllocateLocallyUniqueId, 8 ) \
    SYSCALL_ENTRY( 0x000a, NtAllocateUuids, 32 ) \
    SYSCALL_ENTRY( 0x000b, NtAllocateVirtualMemory, 48 ) \
    SYSCALL_ENTRY( 0x000c, NtAllocateVirtualMemoryEx, 56 ) \
    SYSCALL_ENTRY( 0x000d, NtAreMappedFilesTheSame, 16 ) \
    SYSCALL_ENTRY( 0x000e, NtAssignProcessToJobObject, 16 ) \
    SYSCALL_ENTRY( 0x000f, NtCallbackReturn, 24 ) \
    SYSCALL_ENTRY( 0x0010, NtCancelIoFile, 16 ) \
    SYSCALL_ENTRY( 0x0011, NtCancelIoFileEx, 24 ) \
    SYSCALL_ENTRY( 0x0012, NtCancelSynchronousIoFile, 24 ) \
    SYSCALL_ENTRY( 0x0013, NtCancelTimer, 16 ) \
    SYSCALL_ENTRY( 0x0014, NtClearEvent, 8 ) \
    SYSCALL_ENTRY( 0x0015, NtClose, 8 ) \
    SYSCALL_ENTRY( 0x0016, NtCommitTransaction, 16 ) \
    SYSCALL_ENTRY( 0x0017, NtCompareObjects, 16 ) \
    SYSCALL_ENTRY( 0x0018, NtCompleteConnectPort, 8 ) \
    SYSCALL_ENTRY( 0x0019, NtConnectPort, 64 ) \
    SYSCALL_ENTRY( 0x001a, NtContinue, 16 ) \
    SYSCALL_ENTRY( 0x001b, NtCreateDebugObject, 32 ) \
    SYSCALL_ENTRY( 0x001c, NtCreateDirectoryObject, 24 ) \
    SYSCALL_ENTRY( 0x001d, NtCreateEvent, 40 ) \
    SYSCALL_ENTRY( 0x001e, NtCreateFile, 88 ) \
    SYSCALL_ENTRY( 0x001f, NtCreateIoCompletion, 32 ) \
    SYSCALL_ENTRY( 0x0020, NtCreateJobObject, 24 ) \
    SYSCALL_ENTRY( 0x0021, NtCreateKey, 56 ) \
    SYSCALL_ENTRY( 0x0022, NtCreateKeyTransacted, 64 ) \
    SYSCALL_ENTRY( 0x0023, NtCreateKeyedEvent, 32 ) \
    SYSCALL_ENTRY( 0x0024, NtCreateLowBoxToken, 72 ) \
    SYSCALL_ENTRY( 0x0025, NtCreateMailslotFile, 64 ) \
    SYSCALL_ENTRY( 0x0026, NtCreateMutant, 32 ) \
    SYSCALL_ENTRY( 0x0027, NtCreateNamedPipeFile, 112 ) \
    SYSCALL_ENTRY( 0x0028, NtCreatePagingFile, 32 ) \
    SYSCALL_ENTRY( 0x0029, NtCreatePort, 40 ) \
    SYSCALL_ENTRY( 0x002a, NtCreateSection, 56 ) \
    SYSCALL_ENTRY( 0x002b, NtCreateSemaphore, 40 ) \
    SYSCALL_ENTRY( 0x002c, NtCreateSymbolicLinkObject, 32 ) \
    SYSCALL_ENTRY( 0x002d, NtCreateThread, 64 ) \
    SYSCALL_ENTRY( 0x002e, NtCreateThreadEx, 88 ) \
    SYSCALL_ENTRY( 0x002f, NtCreateTimer, 32 ) \
    SYSCALL_ENTRY( 0x0030, NtCreateToken, 104 ) \
    SYSCALL_ENTRY( 0x0031, NtCreateTransaction, 80 ) \
    SYSCALL_ENTRY( 0x0032, NtCreateUserProcess, 88 ) \
    SYSCALL_ENTRY( 0x0033, NtDebugActiveProcess, 16 ) \
    SYSCALL_ENTRY( 0x0034, NtDebugContinue, 24 ) \
    SYSCALL_ENTRY( 0x0035, NtDelayExecution, 16 ) \
    SYSCALL_ENTRY( 0x0036, NtDeleteAtom, 8 ) \
    SYSCALL_ENTRY( 0x0037, NtDeleteFile, 8 ) \
    SYSCALL_ENTRY( 0x0038, NtDeleteKey, 8 ) \
    SYSCALL_ENTRY( 0x0039, NtDeleteValueKey, 16 ) \
    SYSCALL_ENTRY( 0x003a, NtDeviceIoControlFile, 80 ) \
    SYSCALL_ENTRY( 0x003b, NtDisplayString, 8 ) \
    SYSCALL_ENTRY( 0x003c, NtDuplicateObject, 56 ) \
    SYSCALL_ENTRY( 0x003d, NtDuplicateToken, 48 ) \
    SYSCALL_ENTRY( 0x003e, NtEnumerateKey, 48 ) \
    SYSCALL_ENTRY( 0x003f, NtEnumerateValueKey, 48 ) \
    SYSCALL_ENTRY( 0x0040, NtFilterToken, 48 ) \
    SYSCALL_ENTRY( 0x0041, NtFindAtom, 24 ) \
    SYSCALL_ENTRY( 0x0042, NtFlushBuffersFile, 16 ) \
    SYSCALL_ENTRY( 0x0043, NtFlushInstructionCache, 24 ) \
    SYSCALL_ENTRY( 0x0044, NtFlushKey, 8 ) \
    SYSCALL_ENTRY( 0x0045, NtFlushProcessWriteBuffers, 0 ) \
    SYSCALL_ENTRY( 0x0046, NtFlushVirtualMemory, 32 ) \
    SYSCALL_ENTRY( 0x0047, NtFreeVirtualMemory, 32 ) \
    SYSCALL_ENTRY( 0x0048, NtFsControlFile, 80 ) \
    SYSCALL_ENTRY( 0x0049, NtGetContextThread, 16 ) \
    SYSCALL_ENTRY( 0x004a, NtGetCurrentProcessorNumber, 0 ) \
    SYSCALL_ENTRY( 0x004b, NtGetNextThread, 48 ) \
    SYSCALL_ENTRY( 0x004c, NtGetNlsSectionPtr, 40 ) \
    SYSCALL_ENTRY( 0x004d, NtGetWriteWatch, 56 ) \
    SYSCALL_ENTRY( 0x004e, NtImpersonateAnonymousToken, 8 ) \
    SYSCALL_ENTRY( 0x004f, NtInitializeNlsFiles, 24 ) \
    SYSCALL_ENTRY( 0x0050, NtInitiatePowerAction, 32 ) \
    SYSCALL_ENTRY( 0x0051, NtIsProcessInJob, 16 ) \
    SYSCALL_ENTRY( 0x0052, NtListenPort, 16 ) \
    SYSCALL_ENTRY( 0x0053, NtLoadDriver, 8 ) \
    SYSCALL_ENTRY( 0x0054, NtLoadKey, 16 ) \
    SYSCALL_ENTRY( 0x0055, NtLoadKey2, 24 ) \
    SYSCALL_ENTRY( 0x0056, NtLoadKeyEx, 64 ) \
    SYSCALL_ENTRY( 0x0057, NtLockFile, 80 ) \
    SYSCALL_ENTRY( 0x0058, NtLockVirtualMemory, 32 ) \
    SYSCALL_ENTRY( 0x0059, NtMakeTemporaryObject, 8 ) \
    SYSCALL_ENTRY( 0x005a, NtMapViewOfSection, 80 ) \
    SYSCALL_ENTRY( 0x005b, NtMapViewOfSectionEx, 72 ) \
    SYSCALL_ENTRY( 0x005c, NtNotifyChangeDirectoryFile, 72 ) \
    SYSCALL_ENTRY( 0x005d, NtNotifyChangeKey, 80 ) \
    SYSCALL_ENTRY( 0x005e, NtNotifyChangeMultipleKeys, 96 ) \
    SYSCALL_ENTRY( 0x005f, NtOpenDirectoryObject, 24 ) \
    SYSCALL_ENTRY( 0x0060, NtOpenEvent, 24 ) \
    SYSCALL_ENTRY( 0x0061, NtOpenFile, 48 ) \
    SYSCALL_ENTRY( 0x0062, NtOpenIoCompletion, 24 ) \
    SYSCALL_ENTRY( 0x0063, NtOpenJobObject, 24 ) \
    SYSCALL_ENTRY( 0x0064, NtOpenKey, 24 ) \
    SYSCALL_ENTRY( 0x0065, NtOpenKeyEx, 32 ) \
    SYSCALL_ENTRY( 0x0066, NtOpenKeyTransacted, 32 ) \
    SYSCALL_ENTRY( 0x0067, NtOpenKeyTransactedEx, 40 ) \
    SYSCALL_ENTRY( 0x0068, NtOpenKeyedEvent, 24 ) \
    SYSCALL_ENTRY( 0x0069, NtOpenMutant, 24 ) \
    SYSCALL_ENTRY( 0x006a, NtOpenProcess, 32 ) \
    SYSCALL_ENTRY( 0x006b, NtOpenProcessToken, 24 ) \
    SYSCALL_ENTRY( 0x006c, NtOpenProcessTokenEx, 32 ) \
    SYSCALL_ENTRY( 0x006d, NtOpenSection, 24 ) \
    SYSCALL_ENTRY( 0x006e, NtOpenSemaphore, 24 ) \
    SYSCALL_ENTRY( 0x006f, NtOpenSymbolicLinkObject, 24 ) \
    SYSCALL_ENTRY( 0x0070, NtOpenThread, 32 ) \
    SYSCALL_ENTRY( 0x0071, NtOpenThreadToken, 32 ) \
    SYSCALL_ENTRY( 0x0072, NtOpenThreadTokenEx, 40 ) \
    SYSCALL_ENTRY( 0x0073, NtOpenTimer, 24 ) \
    SYSCALL_ENTRY( 0x0074, NtPowerInformation, 40 ) \
    SYSCALL_ENTRY( 0x0075, NtPrivilegeCheck, 24 ) \
    SYSCALL_ENTRY( 0x0076, NtProtectVirtualMemory, 40 ) \
    SYSCALL_ENTRY( 0x0077, NtPulseEvent, 16 ) \
    SYSCALL_ENTRY( 0x0078, NtQueryAttributesFile, 16 ) \
    SYSCALL_ENTRY( 0x0079, NtQueryDefaultLocale, 16 ) \
    SYSCALL_ENTRY( 0x007a, NtQueryDefaultUILanguage, 8 ) \
    SYSCALL_ENTRY( 0x007b, NtQueryDirectoryFile, 88 ) \
    SYSCALL_ENTRY( 0x007c, NtQueryDirectoryObject, 56 ) \
    SYSCALL_ENTRY( 0x007d, NtQueryEaFile, 72 ) \
    SYSCALL_ENTRY( 0x007e, NtQueryEvent, 40 ) \
    SYSCALL_ENTRY( 0x007f, NtQueryFullAttributesFile, 16 ) \
    SYSCALL_ENTRY( 0x0080, NtQueryInformationAtom, 40 ) \
    SYSCALL_ENTRY( 0x0081, NtQueryInformationFile, 40 ) \
    SYSCALL_ENTRY( 0x0082, NtQueryInformationJobObject, 40 ) \
    SYSCALL_ENTRY( 0x0083, NtQueryInformationProcess, 40 ) \
    SYSCALL_ENTRY( 0x0084, NtQueryInformationThread, 40 ) \
    SYSCALL_ENTRY( 0x0085, NtQueryInformationToken, 40 ) \
    SYSCALL_ENTRY( 0x0086, NtQueryInstallUILanguage, 8 ) \
    SYSCALL_ENTRY( 0x0087, NtQueryIoCompletion, 40 ) \
    SYSCALL_ENTRY( 0x0088, NtQueryKey, 40 ) \
    SYSCALL_ENTRY( 0x0089, NtQueryLicenseValue, 40 ) \
    SYSCALL_ENTRY( 0x008a, NtQueryMultipleValueKey, 48 ) \
    SYSCALL_ENTRY( 0x008b, NtQueryMutant, 40 ) \
    SYSCALL_ENTRY( 0x008c, NtQueryObject, 40 ) \
    SYSCALL_ENTRY( 0x008d, NtQueryPerformanceCounter, 16 ) \
    SYSCALL_ENTRY( 0x008e, NtQuerySection, 40 ) \
    SYSCALL_ENTRY( 0x008f, NtQuerySecurityObject, 40 ) \
    SYSCALL_ENTRY( 0x0090, NtQuerySemaphore, 40 ) \
    SYSCALL_ENTRY( 0x0091, NtQuerySymbolicLinkObject, 24 ) \
    SYSCALL_ENTRY( 0x0092, NtQuerySystemEnvironmentValue, 32 ) \
    SYSCALL_ENTRY( 0x0093, NtQuerySystemEnvironmentValueEx, 40 ) \
    SYSCALL_ENTRY( 0x0094, NtQuerySystemInformation, 32 ) \
    SYSCALL_ENTRY( 0x0095, NtQuerySystemInformationEx, 48 ) \
    SYSCALL_ENTRY( 0x0096, NtQuerySystemTime, 8 ) \
    SYSCALL_ENTRY( 0x0097, NtQueryTimer, 40 ) \
    SYSCALL_ENTRY( 0x0098, NtQueryTimerResolution, 24 ) \
    SYSCALL_ENTRY( 0x0099, NtQueryValueKey, 48 ) \
    SYSCALL_ENTRY( 0x009a, NtQueryVirtualMemory, 48 ) \
    SYSCALL_ENTRY( 0x009b, NtQueryVolumeInformationFile, 40 ) \
    SYSCALL_ENTRY( 0x009c, NtQueueApcThread, 40 ) \
    SYSCALL_ENTRY( 0x009d, NtRaiseException, 24 ) \
    SYSCALL_ENTRY( 0x009e, NtRaiseHardError, 48 ) \
    SYSCALL_ENTRY( 0x009f, NtReadFile, 72 ) \
    SYSCALL_ENTRY( 0x00a0, NtReadFileScatter, 72 ) \
    SYSCALL_ENTRY( 0x00a1, NtReadVirtualMemory, 40 ) \
    SYSCALL_ENTRY( 0x00a2, NtRegisterThreadTerminatePort, 8 ) \
    SYSCALL_ENTRY( 0x00a3, NtReleaseKeyedEvent, 32 ) \
    SYSCALL_ENTRY( 0x00a4, NtReleaseMutant, 16 ) \
    SYSCALL_ENTRY( 0x00a5, NtReleaseSemaphore, 24 ) \
    SYSCALL_ENTRY( 0x00a6, NtRemoveIoCompletion, 40 ) \
    SYSCALL_ENTRY( 0x00a7, NtRemoveIoCompletionEx, 48 ) \
    SYSCALL_ENTRY( 0x00a8, NtRemoveProcessDebug, 16 ) \
    SYSCALL_ENTRY( 0x00a9, NtRenameKey, 16 ) \
    SYSCALL_ENTRY( 0x00aa, NtReplaceKey, 24 ) \
    SYSCALL_ENTRY( 0x00ab, NtReplyWaitReceivePort, 32 ) \
    SYSCALL_ENTRY( 0x00ac, NtRequestWaitReplyPort, 24 ) \
    SYSCALL_ENTRY( 0x00ad, NtResetEvent, 16 ) \
    SYSCALL_ENTRY( 0x00ae, NtResetWriteWatch, 24 ) \
    SYSCALL_ENTRY( 0x00af, NtRestoreKey, 24 ) \
    SYSCALL_ENTRY( 0x00b0, NtResumeProcess, 8 ) \
    SYSCALL_ENTRY( 0x00b1, NtResumeThread, 16 ) \
    SYSCALL_ENTRY( 0x00b2, NtRollbackTransaction, 16 ) \
    SYSCALL_ENTRY( 0x00b3, NtSaveKey, 16 ) \
    SYSCALL_ENTRY( 0x00b4, NtSecureConnectPort, 72 ) \
    SYSCALL_ENTRY( 0x00b5, NtSetContextThread, 16 ) \
    SYSCALL_ENTRY( 0x00b6, NtSetDebugFilterState, 24 ) \
    SYSCALL_ENTRY( 0x00b7, NtSetDefaultLocale, 16 ) \
    SYSCALL_ENTRY( 0x00b8, NtSetDefaultUILanguage, 8 ) \
    SYSCALL_ENTRY( 0x00b9, NtSetEaFile, 32 ) \
    SYSCALL_ENTRY( 0x00ba, NtSetEvent, 16 ) \
    SYSCALL_ENTRY( 0x00bb, NtSetInformationDebugObject, 40 ) \
    SYSCALL_ENTRY( 0x00bc, NtSetInformationFile, 40 ) \
    SYSCALL_ENTRY( 0x00bd, NtSetInformationJobObject, 32 ) \
    SYSCALL_ENTRY( 0x00be, NtSetInformationKey, 32 ) \
    SYSCALL_ENTRY( 0x00bf, NtSetInformationObject, 32 ) \
    SYSCALL_ENTRY( 0x00c0, NtSetInformationProcess, 32 ) \
    SYSCALL_ENTRY( 0x00c1, NtSetInformationThread, 32 ) \
    SYSCALL_ENTRY( 0x00c2, NtSetInformationToken, 32 ) \
    SYSCALL_ENTRY( 0x00c3, NtSetInformationVirtualMemory, 48 ) \
    SYSCALL_ENTRY( 0x00c4, NtSetIntervalProfile, 16 ) \
    SYSCALL_ENTRY( 0x00c5, NtSetIoCompletion, 40 ) \
    SYSCALL_ENTRY( 0x00c6, NtSetLdtEntries, 32 ) \
    SYSCALL_ENTRY( 0x00c7, NtSetSecurityObject, 24 ) \
    SYSCALL_ENTRY( 0x00c8, NtSetSystemInformation, 24 ) \
    SYSCALL_ENTRY( 0x00c9, NtSetSystemTime, 16 ) \
    SYSCALL_ENTRY( 0x00ca, NtSetThreadExecutionState, 16 ) \
    SYSCALL_ENTRY( 0x00cb, NtSetTimer, 56 ) \
    SYSCALL_ENTRY( 0x00cc, NtSetTimerResolution, 24 ) \
    SYSCALL_ENTRY( 0x00cd, NtSetValueKey, 48 ) \
    SYSCALL_ENTRY( 0x00ce, NtSetVolumeInformationFile, 40 ) \
    SYSCALL_ENTRY( 0x00cf, NtShutdownSystem, 8 ) \
    SYSCALL_ENTRY( 0x00d0, NtSignalAndWaitForSingleObject, 32 ) \
    SYSCALL_ENTRY( 0x00d1, NtSuspendProcess, 8 ) \
    SYSCALL_ENTRY( 0x00d2, NtSuspendThread, 16 ) \
    SYSCALL_ENTRY( 0x00d3, NtSystemDebugControl, 48 ) \
    SYSCALL_ENTRY( 0x00d4, NtTerminateJobObject, 16 ) \
    SYSCALL_ENTRY( 0x00d5, NtTerminateProcess, 16 ) \
    SYSCALL_ENTRY( 0x00d6, NtTerminateThread, 16 ) \
    SYSCALL_ENTRY( 0x00d7, NtTestAlert, 0 ) \
    SYSCALL_ENTRY( 0x00d8, NtTraceControl, 48 ) \
    SYSCALL_ENTRY( 0x00d9, NtUnloadDriver, 8 ) \
    SYSCALL_ENTRY( 0x00da, NtUnloadKey, 8 ) \
    SYSCALL_ENTRY( 0x00db, NtUnlockFile, 40 ) \
    SYSCALL_ENTRY( 0x00dc, NtUnlockVirtualMemory, 32 ) \
    SYSCALL_ENTRY( 0x00dd, NtUnmapViewOfSection, 16 ) \
    SYSCALL_ENTRY( 0x00de, NtUnmapViewOfSectionEx, 24 ) \
    SYSCALL_ENTRY( 0x00df, NtWaitForAlertByThreadId, 16 ) \
    SYSCALL_ENTRY( 0x00e0, NtWaitForDebugEvent, 32 ) \
    SYSCALL_ENTRY( 0x00e1, NtWaitForKeyedEvent, 32 ) \
    SYSCALL_ENTRY( 0x00e2, NtWaitForMultipleObjects, 40 ) \
    SYSCALL_ENTRY( 0x00e3, NtWaitForSingleObject, 24 ) \
    SYSCALL_ENTRY( 0x00e4, NtWriteFile, 72 ) \
    SYSCALL_ENTRY( 0x00e5, NtWriteFileGather, 72 ) \
    SYSCALL_ENTRY( 0x00e6, NtWriteVirtualMemory, 40 ) \
    SYSCALL_ENTRY( 0x00e7, NtYieldExecution, 0 ) \
    SYSCALL_ENTRY( 0x00e8, __wine_dbg_ftrace, 24 ) \
    SYSCALL_ENTRY( 0x00e9, __wine_needs_override_large_address_aware, 0 ) \
    SYSCALL_ENTRY( 0x00ea, __wine_set_unix_env, 16 ) \
    SYSCALL_ENTRY( 0x00eb, wine_nt_to_unix_file_name, 32 ) \
    SYSCALL_ENTRY( 0x00ec, wine_unix_to_nt_file_name, 24 )
