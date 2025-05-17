// Prevent console window in addition to Slint window in Windows release builds when, e.g., starting the app via file manager. Ignored on other platforms.
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod randomizer;

use lazy_static::lazy_static;
use libloading::{Library, Symbol};
use random_string::generate;
use std::ffi::OsStr;
use std::os::windows::ffi::OsStrExt;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::thread;
use std::{error::Error, rc::Rc};

lazy_static! {
    static ref LLIB: Library = {
        let lib = unsafe { Library::new("randomizer_util.dll").unwrap() };
        lib
    };
}
// Type definitions for the DLL functions
// type FindProcessIdByNameRaw = unsafe extern "C" fn(*const u16) -> u32;
// type FindMainWindowByPID = unsafe extern "C" fn(u32) -> HWND;
// type SetRandomTitle = unsafe extern "C" fn(HWND, *const u16);
type RenameWindowTitleFn =
    unsafe extern "C" fn(target_process_name: *const u16, new_window_title: *const u16);

const FULL_ASCII: &str = "!\x22#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\r";

slint::include_modules!();

fn main() -> Result<(), Box<dyn Error>> {
    let version = env!("CARGO_PKG_VERSION");

    // Load the functions
    // let find_process_id: Symbol<FindProcessIdByNameRaw> =
    //     unsafe { LLIB.get(b"FindProcessIdByNameRaw")? };
    // let find_main_window: Symbol<FindMainWindowByPID> =
    //     unsafe { LLIB.get(b"FindMainWindowByPID")? };
    // let set_random_title: Symbol<SetRandomTitle> = unsafe { LLIB.get(b"SetRandomTitle")? };

    let rename_windows_title: Symbol<RenameWindowTitleFn> =
        unsafe { LLIB.get(b"RenameWindowTitleByProcessName")? };

    let running = Arc::new(AtomicBool::new(true));
    let flag = Arc::clone(&running);

    let ui = AppWindow::new()?;
    ui.set_version(version.to_string().into());
    let model = Rc::new(slint::VecModel::default());

    model.set_vec(randomizer::get_processes_names().unwrap_or_default());

    ui.set_processes(model.clone().into());

    {
        let ui = ui.clone_strong();
        ui.on_randomizer(move |target_process, title_len| {
            flag.store(true, Ordering::Relaxed); // Signal the thread to start
            let target_process = target_process.to_string();
            let title_len = title_len as usize;

            let process_name_w: Vec<u16> = OsStr::new(target_process.as_str())
                .encode_wide()
                .chain(std::iter::once(0)) // Null-terminate
                .collect();
            // let pid = unsafe { find_process_id(process_name_w.as_ptr()) };

            // if pid == 0 {
            //     return;
            // }

            // let hwnd = unsafe { find_main_window(pid) };
            // if hwnd.is_null() {
            //     return;
            // }
            // Cast HWND to isize for thread safety
            // let hwnd_val = hwnd as isize;
            let rename_windows_title = rename_windows_title.clone();
            let flag = flag.clone();
            thread::spawn(move || {
                while flag.load(Ordering::Relaxed) {
                    // let hwnd = hwnd_val as HWND;
                    let new_title = generate(title_len, FULL_ASCII);
                    let new_title_w: Vec<u16> = OsStr::new(new_title.as_str())
                        .encode_wide()
                        .chain(std::iter::once(0)) // Null-terminate
                        .collect();
                    // unsafe { set_random_title(hwnd, new_title_w.as_ptr()) };
                    unsafe { rename_windows_title(process_name_w.as_ptr(), new_title_w.as_ptr()) };
                    thread::sleep(std::time::Duration::from_millis(100));
                }
            });
        });
    }

    let flag = running.clone();
    ui.on_stop_randomizer(move || {
        flag.store(false, Ordering::Relaxed); // Signal the thread to stop
    });

    ui.on_visit_developer_github(move || {
        open::that("https://github.com/un4ckn0wl3z").unwrap();
    });

    ui.run()?;

    Ok(())
}
