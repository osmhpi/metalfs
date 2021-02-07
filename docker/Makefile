export VIVADO_EDITION ?= webpack
export PSL ?= PSL8
PSL_TAG = $(shell echo $(PSL) | tr A-Z a-z)
METAL_TAG ?= $(shell if [ -z "`git status --porcelain`" ]; then git rev-parse --short HEAD; else echo dirty; fi)
export METAL_TAG := ${METAL_TAG}

docker_build := docker build --build-arg VIVADO_EDITION --build-arg PSL --build-arg METAL_TAG

.PHONY: runtime sdk-base sdk

all-sdk: sdk-base sdk
all-runtime: runtime

git-is-clean:
ifeq '${shell git status --porcelain}' ''
	@ # git is clean
else
	${error Git status is not clean.}
endif


latest-sdk-base: git-is-clean sdk-base
	docker tag metalfs/sdk-base:${METAL_TAG}-${VIVADO_EDITION} 	     metalfs/sdk-base:latest
latest-sdk: git-is-clean sdk
	docker tag metalfs/sdk:${METAL_TAG}-${VIVADO_EDITION}-${PSL_TAG} metalfs/sdk:latest
lastest-runtime: git-is-clean all-runtime
	docker tag metalfs/runtime:${METAL_TAG}                          metalfs/runtime:latest

sdk-base:
	${docker_build} \
		--cache-from metalfs/sdk-base:latest \
		-t metalfs/sdk-base:${METAL_TAG}-${VIVADO_EDITION} \
		-f sdk-base/Dockerfile \
		..

sdk:
	${docker_build} \
		--cache-from metalfs/sdk:latest \
		-t metalfs/sdk:${METAL_TAG}-${VIVADO_EDITION}-${PSL_TAG} \
		-f sdk/Dockerfile \
		..

runtime:
	${docker_build} \
		-t metalfs/runtime:${METAL_TAG} \
		-f runtime/Dockerfile \
		..

pull-latest-sdk:
	docker pull metalfs/sdk-base:latest
	docker pull metalfs/sdk:latest
pull-latest-runtime:
	docker pull metalfs/runtime:latest

push-sdk:
	SUBTAG=$$(echo '${METAL_TAG}' | tr -d 'v') ; \
	if [ $$(echo $$SUBTAG | grep -iE '^[0-9]+\.[0-9]+\.[0-9]+$$' | wc -l) -ne 0 ] ; then \
		while [ ! -z "$$SUBTAG" ]; do \
			docker tag metalfs/sdk-base:${METAL_TAG}-${VIVADO_EDITION} metalfs/sdk-base:$$SUBTAG-${VIVADO_EDITION} ; \
			docker tag metalfs/sdk:${METAL_TAG}-${VIVADO_EDITION}-${PSL_TAG} metalfs/sdk:$$SUBTAG-${VIVADO_EDITION}-${PSL_TAG} ; \
			docker push metalfs/sdk-base:$$SUBTAG-${VIVADO_EDITION} ; \
			docker push metalfs/sdk:$$SUBTAG-${VIVADO_EDITION}-${PSL_TAG} ; \
			SUBTAG=$$(echo "$$SUBTAG" | awk '{gsub(/\.?[0-9]+$$/,"",$$1);print}') ; \
		done ; \
	fi

push-runtime:
	docker push metalfs/runtime:${METAL_TAG}

push-latest-sdk: git-is-clean
	docker push metalfs/sdk-base:latest
	docker push metalfs/sdk:latest
push-latest-runtime: git-is-clean
	docker push metalfs/runtime:latest
