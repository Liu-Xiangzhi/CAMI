gcc_preprocess_cmd = ['gcc', 
                    # specify standard
                      '-std=c23',
                    # diable builtin functions, system headers, and macro definitions
                      '-fno-builtin', '-nostdinc', '-undef', 
                    # specify camilibc include directory
                      '-isystem', '/home/liuxiangzhi/projects/cami/camilibc/include',
                    # define cami-specific macros
                      '-D__CAMIC__',
                    # redefine all STDC macros
                      '-U__STDC__', '-D__STDC__=1',
                      '-U__STDC_EMBED_NOT_FOUND__', '-D__STDC_EMBED_NOT_FOUND__=0', 
                      '-U__STDC_EMBED_FOUND__', '-D__STDC_EMBED_FOUND__=1', 
                      '-U__STDC_EMBED_EMPTY__', '-D__STDC_EMBED_EMPTY__=2',
                      '-U__STDC_HOSTED__', '-D__STDC_HOSTED__=1',
                      '-U__STDC_UTF_16__', '-D__STDC_UTF_16__=1',
                      '-U__STDC_UTF_32__', '-D__STDC_UTF_32__=1',
                      '-U__STDC_VERSION__', '-D__STDC_VERSION__=202311L',
                      '-U__STDC_ISO_10646__', '-D__STDC_ISO_10646__=1',
                      '-U__STDC_MB_MIGHT_NEQ_WC__',
                      '-U__STDC_ANALYZABLE__',
                      # floating types are not well supported, related macros are not defined now
                      '-U__STDC_IEC_60559_BFP__',
                      '-U__STDC_IEC_559__',
                      '-U__STDC_IEC_60559_DFP__',
                      '-U__STDC_IEC_60559_COMPLEX__',
                      '-U__STDC_IEC_60559_TYPES__',
                      '-U__STDC_IEC_559_COMPLEX__',
                      '-U__STDC_LIB_EXT1__',
                      '-U__STDC_NO_ATOMICS__', '-D__STDC_NO_ATOMICS__=1',
                      '-U__STDC_NO_COMPLEX__', '-D__STDC_NO_COMPLEX__=1',
                      '-U__STDC_NO_THREADS__', '-D__STDC_NO_THREADS__=1',
                      '-U__STDC_NO_VLA__', '-D__STDC_NO_VLA__=1',
                      '-E',
                      '/home/liuxiangzhi/projects/cami/camic/test/testcases/a.c']

def main(argv : list[str]):
    import subprocess, os, sys
    p = subprocess.run(gcc_preprocess_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        print('\033[31mPreprocess Failed:\033[0m', file=sys.stderr)
        print(p.stderr.decode(), file=sys.stderr)
        print('Note that warnings(from preprocessor of gcc) about undefining of "__STDC_*" macros just can be ignored', file=sys.stderr)
        return -1
    if len(argv) > 1 and argv[1] == 'preprocess':
        if len(argv) > 2 and argv[2] == 'bin':
            with open('/home/liuxiangzhi/projects/cami/camic/_build/default/bin/a.bin', 'wb') as f:
                f.write(p.stdout)
        else:
            print(p.stdout.decode(), end="")
        return 0
    env = os.environ.copy()
    ocamlrunparam = env.get("OCAMLRUNPARAM", "")
    env["OCAMLRUNPARAM"] =  "b" if ocamlrunparam == "" else "b," + ocamlrunparam
    if len(argv) > 1 and argv[1] == 'debug':
        with open('/home/liuxiangzhi/projects/cami/camic/_build/default/bin/a.bin', 'wb') as f:
            f.write(p.stdout.decode().encode('utf-8'))
        subprocess.run(['ocamldebug','-I', '/home/liuxiangzhi/projects/cami/camic/_build/default/lib', '/home/liuxiangzhi/projects/cami/camic/_build/default/bin/main.bc', '/home/liuxiangzhi/projects/cami/camic/_build/default/bin/a.bin'], env=env)
    else:
        p = subprocess.run(['/home/liuxiangzhi/projects/cami/camic/_build/install/default/bin/camic'], input=p.stdout, env=env)
        return p.returncode
    return 0
        

if __name__ == '__main__':
    import sys
    exit(main(sys.argv))

