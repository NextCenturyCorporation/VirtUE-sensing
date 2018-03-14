import sys
import wmi
import pytest
import sysinfo

@pytest.fixture(scope='module')
def winit_id():
    '''
    return the winint process id
    '''
    c = wmi.WMI(find_classes=False)
    proc_id = [i.ProcessId for i in c.Win32_Process(["ProcessId"],name="wininit.exe")][0]
    return proc_id

@pytest.fixture(scope='module')
def privs():
    new_privs = (
        (LookupPrivilegeValue(None, SE_SECURITY_NAME), SE_PRIVILEGE_ENABLED),
        (LookupPrivilegeValue(None, SE_CREATE_PERMANENT_NAME), SE_PRIVILEGE_ENABLED), 
        (LookupPrivilegeValue(None, SE_DEBUG_NAME), SE_PRIVILEGE_ENABLED)
    )  
    success = acquire_privileges(new_privs)
    return success

def test_process_objects(privs):

    success = privs.process_objects()
    assert True == success

    for proc_obj, spi_ary in get_process_objects():
        print(proc_obj)
        for thd_obj in get_thread_objects(proc_obj, spi_ary):
            print(thd_obj)

def test_thread_objects(privs, winit_id):
    '''
    '''
    success = privs.process_objects()
    assert True == success

    pid = winit_id.thread_objects()
    assert pid > 0
        
    for proc_obj, spi_ary in get_process_objects(pid):            
        print(proc_obj)
        for thd_obj in get_thread_objects(proc_obj, spi_ary):
            print(thd_obj)



#        proc_data = proc_obj.Encode()
#        for hdlinfo in get_system_handle_information(pid):            
#            type_name = get_nt_object_type_info(hdlinfo)            
#            object_name = get_nt_object_name_info(hdlinfo)
#            hdl = proc_obj.ToDict()
#            hdl["type_name"] = type_name
#            hdl["object_name"] = object_name
#            serdata = hdlinfo.Encode()
#            print(serdata)
#            new_test_instance = SYSTEM_HANDLE_TABLE_ENTRY_INFO.Decode(serdata)
#            print(hdlinfo)
#        new_proc_obj = SYSTEM_PROCESS_INFORMATION.Decode(proc_data)
#        print(new_proc_obj)         
#                    
#    sys.exit(0)
    
    
    #try:        
        #sbi = get_basic_system_information()
        #print("System Basic Info = {0}\n".format(sbi,))
    
        #for pid in get_process_ids():                    
            #try:
                #proc_obj = get_process_objects(pid)
                #print("Process ID {0} Handle Information: {1}\n".format(pid,proc_obj,))                
                #for dupHandle in get_handle_information(pid):
                    #type_name = get_nt_object_type_info(dupHandle)
                    #object_name = get_nt_object_name_info(dupHandle)            
                    #print("\tHandle={0},type_name={1},object_name={2}\n".format(dupHandle, type_name,object_name,))           
                    #dupHandle.Close()            
            #except pywintypes.error as err:
                #print("Unable to get handle information from pid {0} - {1}\n"
                      #.format(pid, err,))
    #finally:
        #success = release_privileges(new_privs)        
