import os
import glob

def patch_file(filepath, replacements):
    with open(filepath, 'r') as f:
        content = f.read()
    
    modified = content
    for target, rep in replacements.items():
        modified = modified.replace(target, rep)
    
    if modified != content:
        with open(filepath, 'w') as f:
            f.write(modified)
        print(f"Patched {os.path.basename(filepath)}")

def main():
    root = os.path.dirname(os.path.abspath(__file__))
    debug_dir = os.path.join(root, "Debug")
    
    replacements = {
        r'C:\Users\alire\Documents\MCUXpressoIDE_25.6.136\workspace\k32w041amdk6_i2c_polling_transfer': '..',
        '-specs=redlib.specs': '--specs=nano.specs',
        '-D__REDLIB__': '-D__NEWLIB__',
        '-D__MCUXPRESSO': '',
    }
    
    # 1. Patch all subdir.mk files
    for mk in glob.glob(os.path.join(debug_dir, "**/subdir.mk"), recursive=True):
        patch_file(mk, replacements)
        
    # 2. Patch library.ld
    lib_ld = os.path.join(debug_dir, "k32w041amdk6_i2c_polling_transfer_Debug_library.ld")
    if os.path.exists(lib_ld):
        patch_file(lib_ld, {
            '"libcr_nohost_nf.a"': '"libnosys.a"',
            '"libcr_c.a"': '"libc_nano.a"',
            '"libcr_eabihelpers.a"': '"libm.a"',
        })
        
    print("Done patching makefiles!")

if __name__ == '__main__':
    main()
