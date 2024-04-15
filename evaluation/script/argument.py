import os


eval_root = os.path.dirname(os.path.dirname(__file__))
test_suite_dir = os.path.join(eval_root, 'test_suite')
out_dir = os.path.join(eval_root, 'out')
config_path = os.path.join(eval_root, 'config/config.json')
verbose = False
verbose_output_path = os.path.join(out_dir, 'evaluation_output.html')