# 'make' builds everything
# 'make clean' deletes everything except source files and Makefile

CONFIG = config_local.yaml
DOCKER=False

all: kserverd

libraries:
	wget -P tmp bitbucket.org/eigen/eigen/get/3.2.6.tar.gz
	cd tmp && tar -zxvf 3.2.6.tar.gz
	mkdir -p middleware/libraries
	cp -r tmp/eigen-eigen-c58038c56923/Eigen middleware/libraries 

ifeq ($(DOCKER),False)
venv:
	virtualenv venv
	venv/bin/pip install -r requirements.txt
else # No virtualenv required when running in a Docker container
venv:
endif

kserverd: venv libraries
ifeq ($(DOCKER),False)
	venv/bin/python kmake.py kserver -c config/$(CONFIG)
else
	python kmake.py kserver -c config/$(CONFIG)
endif

clean:
	rm -rf tmp

