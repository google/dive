@echo off
git diff --quiet HEAD
if not %ERRORLEVEL%==0 (
    echo FAILED git check diff, git workspace not clean:
    git --no-pager diff -p HEAD
    exit 1
)
echo PASSED git check diff