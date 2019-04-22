TEMPLATE = subdirs
SUBDIRS = \
    tests \
    src \
    app \
    git-version

app.depends += src git-version
tests.depends += src

DISTFILES += LICENSE README.md .gitlab-ci.yml .appveyor.yml .gitignore \
    .clang-format \
    misc/e2designer.desktop \
    Doxyfile \
    docker/flatpak/Dockerfile docker/flatpak/yaml2json.py \
    org.technic93.e2designer.yaml \
    docker/Dockerfile docker-push.sh gitlab-runner-exec.sh
