# System tool 

Setup CI/CD pipeline for sysinfo project

### sysinfo (C)
**C code compilation**

`make build`

**C code lint**

`make lint`

**C binary execution**

`make run`

### mysystem (Python)
**Python script dependancies installation**

`pip install ifaddr`  
`pip install psutil`

**Python script lint**

`pip install pylint`  
`pylint --disable=missing-docstring script/mysystem`

**Python script execution**

`python script/mysystem/mysystem.py`

**Python unit tests**

 `pytest --junitxml=report.xml script/mysystem`
