:; python3 "$(dirname "$0")/cubemx_hook.py" pre "$0"; exit $?
@echo off
python "%~dp0cubemx_hook.py" pre "%0"