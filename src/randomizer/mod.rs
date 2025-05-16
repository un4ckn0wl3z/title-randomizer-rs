#![allow(unused)]

use windows_sys::{Win32::Foundation::*, Win32::System::Diagnostics::ToolHelp::*};

#[derive(Debug, Clone)]
pub struct Process {
    pub pid: i32,
    pub name: slint::SharedString,
}

impl Process {
    pub fn new(pid: i32, name: slint::SharedString) -> Self {
        Process { pid, name }
    }

    pub fn get_name(&self) -> &str {
        &self.name
    }

    pub fn get_pid(&self) -> i32 {
        self.pid
    }
}

pub fn get_processes() -> Option<Vec<Process>> {
    let mut process_list = Vec::new();

    unsafe {
        let h_snapshot: HANDLE = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if h_snapshot == INVALID_HANDLE_VALUE {
            return None;
        }
        let mut process_entry: PROCESSENTRY32 = std::mem::zeroed();
        process_entry.dwSize = std::mem::size_of::<PROCESSENTRY32>() as u32;
        let mut result = Process32First(h_snapshot, &mut process_entry);
        while result != 0 {
            let pid = process_entry.th32ProcessID;
            let name = {
                let exe_bytes = &process_entry.szExeFile;
                let exe_u16: Vec<u16> = exe_bytes
                    .iter()
                    .map(|&c| c as u8 as u16)
                    .take_while(|&c| c != 0)
                    .collect();
                String::from_utf16_lossy(&exe_u16)
            };
            let name = name.trim_end_matches('\0').to_string();
            let process = Process::new(pid as i32, name.into());
            process_list.push(process);
            result = Process32Next(h_snapshot, &mut process_entry);
        }
    };
    if process_list.is_empty() {
        return None;
    }
    Some(process_list)
}

pub fn get_processes_names() -> Option<Vec<slint::SharedString>> {
    let processes = get_processes()?;
    let names = processes.iter().map(|p| p.get_name().into()).collect();
    Some(names)
}
