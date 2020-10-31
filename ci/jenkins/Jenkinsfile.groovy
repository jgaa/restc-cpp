#!/usr/bin/env groovy

pipeline {
    agent { label 'master' }

    environment {
        RESTC_CPP_VERSION = "0.9.2"

        // It is not possible to get the current IP number when running in the sandbox, and
        // Jenkinsfiles always runs in the sandbox.
        // For simplicity, I just put it here (I already wasted 3 hours on this)
        RESTC_CPP_TEST_DOCKER_ADDRESS="192.168.1.180"
    }

    stages {
        stage('Prepare') {
            agent { label 'master' }
            steps {
                sh 'docker-compose -f ./ci/mock-backends/docker-compose.yml up --build  -d'
            }
        }

        stage('Build') {
           parallel {
                stage('Ubuntu Bionic') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.ubuntu-bionic'
                            dir 'ci/jenkins'
                            label 'docker'
                        }
                    }

                    steps {
                        echo "Building on ubuntu-bionic-AMD64 in ${WORKSPACE}"
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
                                echo 'Testing failed'
                                currentBuild.result = 'UNSTABLE'
                            }
                        }
                    }
                }

                stage('Ubuntu Xenial') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.ubuntu-xenial'
                            dir 'ci/jenkins'
                            label 'docker'
                        }
                    }

                    steps {
                        echo "Building on ubuntu-xenial-AMD64 in ${WORKSPACE}"
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
                                echo 'Testing failed'
                                currentBuild.result = 'UNSTABLE'
                            }
                        }
                    }
                }

                 stage('Debian Stretch') {
                    agent {
                        dockerfile {
                            filename 'Dockerfile.debian-stretch'
                            dir 'ci/jenkins'
                            label 'docker'
                        }
                    }

                    steps {
                        echo "Building on debian-stretch-AMD64 in ${WORKSPACE}"
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
                                echo 'Testing failed'
                                currentBuild.result = 'UNSTABLE'
                            }
                        }
                    }
                }
                
                stage('Debian Buster') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-buster'
                            dir 'ci/jenkins'
                            label 'docker'
                        }
                    }

                    steps {
                        echo "Building on debian-buster-AMD64 in ${WORKSPACE}"
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
                                echo 'Testing failed'
                                currentBuild.result = 'UNSTABLE'
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
                        }
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
                                echo 'Testing failed'
                                currentBuild.result = 'UNSTABLE'
                            }
                        }
                    }
                }

//                 stage('Fedora') {
//                     agent {
//                         dockerfile {
//                             filename 'Dockerfile.fedora'
//                             dir 'ci/jenkins'
//                             label 'docker'
//                         }
//                     }
// 
//                     steps {
//                         echo "Building on Fedora in ${WORKSPACE}"
//                         checkout scm
//                         sh 'pwd; ls -la'
//                         sh 'rm -rf build'
//                         sh 'mkdir build'
//                         sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make'
// 
//                         echo 'Getting ready to run tests'
//                         script {
//                             try {
//                                 sh 'cd build && ctest --no-compress-output -T Test'
//                             } catch (exc) {
//                                 echo 'Testing failed'
//                                 currentBuild.result = 'UNSTABLE'
//                             }
//                         }
//                     }
//                 }
//                
//                 stage('Centos7') {
//                     agent {
//                         dockerfile {
//                             filename 'Dockerfile.centos7'
//                             dir 'ci/jenkins'
//                             label 'docker'
//                         }
//                     }
// 
//                     steps {
//                         echo "Building on Centos7 in ${WORKSPACE}"
//                         checkout scm
//                         sh 'pwd; ls -la'
//                         sh 'rm -rf build'
//                         sh 'mkdir build'
//                         sh 'cd build && source scl_source enable devtoolset-7 && cmake -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=/opt/boost .. && make'
// 
//                         echo 'Getting ready to run tests'
//                         script {
//                             try {
//                                 sh 'cd build && ctest --no-compress-output -T Test'
//                             } catch (exc) {
//                                 echo 'Testing failed'
//                                 currentBuild.result = 'UNSTABLE'
//                             }
//                         }
//                     }
//                 }

                stage('macOS') {
                    agent {label 'macos'}

                    environment {
                        CPPFLAGS = "-I/usr/local/opt/openssl/include -I/usr/local/opt/zlib/include -I/usr/local/opt/boost/include/"
                        LDFLAGS = "-L/usr/local/opt/openssl/lib -L/usr/local/opt/zlib/lib -L/usr/local/opt/boost/lib/"
                    }

                    //Dependencies are installed with: brew install openssl boost zlib

                    steps {
                        echo "Building on macos in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DBOOST_LIBRARIES=/usr/local/opt/boost/lib -DBOOSTL_ROOT_DIR=/usr/local/opt/boost -DZLIB_INCLUDE_DIRS=/usr/local/opt/zlib/include/ -DZLIB_LIBRARIES=/usr/local/opt/zlib/lib/ .. && make -j4'

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

                stage('Windows X64 with vcpkg') {

                    agent {label 'windows'}

                     steps {
                        echo "Building on Windows in ${WORKSPACE}"
                        checkout scm

                        bat script: '''
                            PATH=%PATH%;C:\\Program Files\\CMake\\bin;C:\\devel\\vcpkg
                            vcpkg install zlib openssl boost-fusion boost-filesystem boost-log boost-program-options boost-asio boost-date-time boost-chrono boost-coroutine boost-uuid boost-scope-exit --triplet x64-windows
                            if %errorlevel% neq 0 exit /b %errorlevel%
                            rmdir /S /Q build
                            mkdir build
                            cd build
                            cmake -DRESTC_CPP_USE_CPP17=ON -DCMAKE_PREFIX_PATH=C:\\devel\\vcpkg\\installed\\x64-windows\\lib;C:\\devel\\vcpkg\\installed\\x64-windows\\include ..
                            if %errorlevel% neq 0 exit /b %errorlevel%
                            cmake --build . --config Release
                            if %errorlevel% neq 0 exit /b %errorlevel%
                            echo "Build is OK"
                        '''

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                bat script: '''
                                    PATH=%PATH%;C:\\devel\\vcpkg\\installed\\x64-windows\\bin;C:\\Program Files\\CMake\\bin
                                    cd build
                                    ctest -C Release
                                    if %errorlevel% neq 0 exit /b %errorlevel%
                                '''
                            } catch (exc) {
                                echo 'Testing failed'
                                currentBuild.result = 'UNSTABLE'
                            }
                        }
                    }
                }
            }

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

