#!/usr/bin/env groovy

pipeline {
    agent { label 'main' }

    environment {
        RESTC_CPP_VERSION = "1.0.0"

        // It is not possible to get the current IP number when running in the sandbox, and
        // Jenkinsfiles always runs in the sandbox.
        // For simplicity, I just put it here (I already wasted 3 hours on this)
        RESTC_CPP_TEST_DOCKER_ADDRESS="192.168.1.55"
        CTEST_OUTPUT_ON_FAILURE=1
    }

    stages {
        stage('Prepare') {
            agent { label 'main' }
            steps {
                sh 'docker-compose -f ./ci/mock-backends/docker-compose.yml up --build  -d'
            }
        }

        stage('Build') {
           parallel {

                stage('macOS') {
                    agent {label 'macos'}

                    // environment {
                    //     CPPFLAGS = "-I/usr/local/opt/openssl/include -I/usr/local/opt/zlib/include -I/usr/local/opt/boost/include/"
                    //     LDFLAGS = "-L/usr/local/opt/openssl/lib -L/usr/local/opt/zlib/lib -L/usr/local/opt/boost/lib/"
                    // }

                    steps {
                        echo "Building on macos in ${WORKSPACE}"
                        sh 'brew install openssl boost zlib rapidjson googletest cmake ninja'
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {
                                echo 'Testing failed'
                                currentBuild.result = 'UNSTABLE'
                            }
                        }
                    }
                }

                stage('Ubuntu Plucky') {
                    agent {
                        dockerfile {
                            filename 'Dockerfile.ubuntu-plucky'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on ubuntu-plucky-AMD64 in ${NODE_NAME} --> ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release  .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                stage('Ubuntu Plucky MT CTX') {
                    agent {
                        dockerfile {
                            filename 'Dockerfile.ubuntu-plucky'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on ubuntu-plucky-MT-AMD64 in ${NODE_NAME} --> ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DRESTC_CPP_THREADED_CTX=ON -DCMAKE_BUILD_TYPE=Release  .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                stage('Ubuntu Noble') {
                    agent {
                        dockerfile {
                            filename 'Dockerfile.ubuntu-noble'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on ubuntu-noble-AMD64 in ${NODE_NAME} --> ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release  .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                stage('Ubuntu Noble MT CTX') {
                    agent {
                        dockerfile {
                            filename 'Dockerfile.ubuntu-noble'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on ubuntu-noble-AMD64 in ${NODE_NAME} --> ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DRESTC_CPP_THREADED_CTX=ON -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                stage('Ubuntu Jammy') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.ubuntu-jammy'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on ubuntu-jammy-AMD64 in ${NODE_NAME} --> ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                stage('Ubuntu Jammy MT CTX') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.ubuntu-jammy'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on ubuntu-jammy-AMD64 in ${NODE_NAME} --> ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DRESTC_CPP_THREADED_CTX=ON -DCMAKE_BUILD_TYPE=Release  .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                stage('Ubuntu Jammy') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.ubuntu-jammy'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on ubuntu-jammy-AMD64 in ${NODE_NAME} --> ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }


                stage('Debian Bullseye') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-bullseye'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on debian-bullseye-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                 stage('Debian Bullseye MT CTX') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-bullseye'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on debian-bullseye-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DRESTC_CPP_THREADED_CTX=ON  -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                stage('Debian Bookworm') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-bookworm'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on debian-bookworm-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                 stage('Debian Bookworm MT CTX') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-bookworm'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on debian-bookworm-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DRESTC_CPP_THREADED_CTX=ON -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                stage('Debian Trixie') {
                    agent {
                        dockerfile {
                            filename 'Dockerfile.debian-trixie'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on debian-trixie-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                stage('Debian Trixie MT CTX') {
                    agent {
                        dockerfile {
                            filename 'Dockerfile.debian-trixie'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on debian-trixie-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DRESTC_CPP_THREADED_CTX=ON -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                stage('Debian Testing') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-testing'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on debian-testing-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                stage('Debian Testing MT CTX') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-testing'
                            dir 'ci/jenkins'
                            label 'docker'
                            args '-u root'
                        }
                    }

                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on debian-testing-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DRESTC_CPP_THREADED_CTX=ON -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

                stage('Fedora CTX') {
                    agent {
                        dockerfile {
                            filename 'Dockerfile.fedora'
                            dir 'ci/jenkins'
                            label 'docker'
                        }
                    }

                    steps {
                        echo "Building on Fedora in ${WORKSPACE}"
                        checkout scm
                        sh 'set -x'
                        sh 'rm -rf build-fedora'
                        sh 'mkdir build-fedora'
                        sh 'cd build-fedora && cmake -DRESTC_CPP_THREADED_CTX=ON -DCMAKE_BUILD_TYPE=Release .. && cmake --build . -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build-fedora && ctest --no-compress-output -T Test'
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }

                        sh 'rm -rf build-fedora'
                    }
                }
                
                stage('Windows X64 with vcpkg') {

                    agent {label 'windows'}

                    options {
                        // vcpkg now installs and compiles pretty much everything that exists on github if you ask it to prepare boost and openssl.
                        // It's becoming as bad as js and npm.
                        timeout(time: 60, unit: "MINUTES")
                    }

                     steps {
                        echo "Building on Windows in ${WORKSPACE}"
                        checkout scm

                        bat script: '''
                            PATH=C:\\src\\vcpkg;%PATH%;C:\\Program Files\\CMake\\bin;C:\\Program Files\\Git\\bin
                            git -C C:\\src\\vcpkg pull --rebase
                            vcpkg upgrade --no-dry-run
                            vcpkg remove --outdated
                            vcpkg integrate install
                            vcpkg install rapidjson gtest zlib openssl boost-program-options boost-filesystem boost-date-time boost-coroutine boost-context boost-chrono boost-asio boost-system boost-log --triplet x64-windows
                            if %errorlevel% neq 0 exit /b %errorlevel%
                            rmdir /S /Q build
                            mkdir build
                            cd build
                            cmake -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake ..
                            if %errorlevel% neq 0 exit /b %errorlevel%
                            cmake --build . --config Release
                            if %errorlevel% neq 0 exit /b %errorlevel%
                            echo "Build is OK"
                        '''

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                bat script: '''
                                    PATH=%PATH%;C:\\src\\vcpkg\\installed\\x64-windows\\bin;C:\\Program Files\\CMake\\bin
                                    cd build
                                    ctest -C Release
                                    if %errorlevel% neq 0 exit /b %errorlevel%
                                '''
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }


                stage('Windows X64 with vcpkg MT CTX') {

                    agent {label 'windows'}

                    options {
                        // vcpkg now installs and compiles pretty much everything that exists on github if you ask it to prepare boost and openssl.
                        // It's becoming as bad as js and npm.
                        timeout(time: 60, unit: "MINUTES")
                    }

                     steps {
                        echo "Building on Windows in ${WORKSPACE}"
                        checkout scm

                        bat script: '''
                            PATH=C:\\src\\vcpkg;%PATH%;C:\\Program Files\\CMake\\bin;C:\\Program Files\\Git\\bin
                            git -C C:\\src\\vcpkg pull --rebase
                            vcpkg upgrade --no-dry-run
                            vcpkg remove --outdated
                            vcpkg integrate install
                            vcpkg install rapidjson gtest zlib openssl boost-program-options boost-filesystem boost-date-time boost-coroutine boost-context boost-chrono boost-asio boost-system boost-log --triplet x64-windows
                            if %errorlevel% neq 0 exit /b %errorlevel%
                            rmdir /S /Q build
                            mkdir build
                            cd build
                            cmake -DRESTC_CPP_THREADED_CTX=ON -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake ..
                            if %errorlevel% neq 0 exit /b %errorlevel%
                            cmake --build . --config Release
                            if %errorlevel% neq 0 exit /b %errorlevel%
                            echo "Build is OK"
                        '''

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                bat script: '''
                                    PATH=%PATH%;C:\\src\\vcpkg\\installed\\x64-windows\\bin;C:\\Program Files\\CMake\\bin
                                    cd build
                                    ctest -C Release
                                    if %errorlevel% neq 0 exit /b %errorlevel%
                                '''
                            } catch (exc) {

                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }

            } // parallel

            post {
                always {
                    echo 'Shutting down test containers.'
                    sh 'docker-compose -f ./ci/mock-backends/docker-compose.yml down'
                    sh 'docker-compose -f ./ci/mock-backends/docker-compose.yml rm'
                }
            }
        }
    }
}

