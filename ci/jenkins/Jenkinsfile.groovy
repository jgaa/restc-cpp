#!/usr/bin/env groovy

pipeline {
    agent { label 'master' }

    environment {
        RESTC_CPP_VERSION = "0.9.1"

        // It is not possible to get the current IP number when running in the sandbox, and
        // Jenkinsfiles always runs in the sandbox.
        // For simplicity, I just put it here (I already wastes 3 hours on this)
        RESTC_CPP_TEST_DOCKER_ADDRESS="10.201.0.11"
    }

    stages {
        stage('Prepare') {
            steps {
                sh './create-and-run-containers.sh'
                sh 'docker ps'
            }
        }

        stage('Build') {
           parallel {
                stage('Docker-build inside: ubuntu-xenial') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.ubuntu-xenial'
                            dir 'ci/jenkins'
                            label 'master'
                        }
                    }

                    steps {
                        echo "Building on ubuntu-xenial-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j2 && ctest --no-compress-output -T Test'
                    }
                }

                stage('Docker-build inside: debian-stretch') {
                    agent {
                        dockerfile {
                            filename 'Dockerfile.debian-stretch'
                            dir 'ci/jenkins'
                            label 'master'
                        }
                    }

                    steps {
                        echo "Building on debian-stretch-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j2 && ctest --no-compress-output -T Test'
                    }
                }

                stage('Docker-build inside: debian-testing') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-testing'
                            dir 'ci/jenkins'
                            label 'master'
                        }
                    }

                    steps {
                        echo "Building on debian-testing-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j2 && ctest --no-compress-output -T Test'
                    }
                }

                stage('macOS-build: ') {
                    agent {label 'macos'}

                    environment {
                        CPPFLAGS = "-I/usr/local/opt/openssl/include -I/usr/local/opt/zlib/include -I/usr/local/opt/boost/include/"
                        LDFLAGS = "-L/usr/local/opt/openssl/lib -L/usr/local/opt/zlib/lib -L/usr/local/opt/boost/lib/"
                    }

                    // Dependencies are installed with: brew install openssl boost zlib

                    steps {
                        echo "Building on ubuntu-xenial-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DBOOST_LIBRARIES=/usr/local/opt/boost/lib -DBOOSTL_ROOT_DIR=/usr/local/opt/boost -DZLIB_INCLUDE_DIRS=/usr/local/opt/zlib/include/ -DZLIB_LIBRARIES=/usr/local/opt/zlib/lib/ .. && make -j4 && ctest --no-compress-output -T Test'
                    }
                }

                stage('Windows-MSVCx64') {

                    agent {label 'windows'}

                     steps {
                        echo "Building on Windows in ${WORKSPACE}"
                        checkout scm

                        bat script: '''

                        PATH=%PATH%;C:\\devel\\zlib-1.2.11\\Release;C:\\devel\\openssl\\lib-x64\\bin;C:\\local\\boost_1_64_0\\lib64-msvc-14.1
                        rmdir /S /Q build
                        mkdir build
                        cd build
                        cmake -DCMAKE_PREFIX_PATH=c:/devel/zlib-1.2.11;c:/devel/zlib-1.2.11/Release;c:/devel/openssl;c:/devel/boost_1_64_0 -G "Visual Studio 15 Win64" ..
                        if %errorlevel% neq 0 exit /b %errorlevel%
                        cmake --build . --config Release
                        if %errorlevel% neq 0 exit /b %errorlevel%
                        ctest -C Release
                        if %errorlevel% neq 0 exit /b %errorlevel%

                        echo "Everything is OK"
                        '''
                    }
                }

                stage('Windows-MSVCx64-cpp17') {

                    agent {label 'windows'}

                     steps {
                        echo "Building on Windows in ${WORKSPACE}"
                        checkout scm

                        bat script: '''

                        PATH=%PATH%;C:\\devel\\zlib-1.2.11\\Release;C:\\devel\\openssl\\lib-x64\\bin;C:\\local\\boost_1_64_0\\lib64-msvc-14.1
                        rmdir /S /Q build
                        mkdir build
                        cd build
                        cmake -DRESTC_CPP_USE_CPP17=ON -DCMAKE_PREFIX_PATH=c:/devel/zlib-1.2.11;c:/devel/zlib-1.2.11/Release;c:/devel/openssl;c:/devel/boost_1_64_0 -G "Visual Studio 15 Win64" ..
                        if %errorlevel% neq 0 exit /b %errorlevel%
                        cmake --build . --config Release
                        if %errorlevel% neq 0 exit /b %errorlevel%
                        ctest -C Release
                        if %errorlevel% neq 0 exit /b %errorlevel%

                        echo "Everything is OK"
                        '''
                    }
                }

                stage('Docker-build inside: Fedora') {
                    agent {
                        dockerfile {
                            filename 'Dockerfile.fedora'
                            dir 'ci/jenkins'
                            label 'master'
                        }
                    }

                    steps {
                        echo "Building on Fedora in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j2 && ctest --no-compress-output -T Test'
                    }
                }
            }
        }

        stage('Clean-up') {
            steps {
                sh 'docker stop restc-cpp-squid restc-cpp-nginx restc-cpp-json'
            }
        }
    }
}

