#!/usr/bin/env groovy

pipeline {
    agent { label 'master' }

    environment {
        RESTC_CPP_VERSION = "0.97.0"

        // It is not possible to get the current IP number when running in the sandbox, and
        // Jenkinsfiles always runs in the sandbox.
        // For simplicity, I just put it here (I already wasted 3 hours on this)
        RESTC_CPP_TEST_DOCKER_ADDRESS="192.168.1.131"
        CTEST_OUTPUT_ON_FAILURE=1
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
// Broken:  java.io.IOException: Failed to run image '692f7cce9b970633dba347a9aaf12846429c073f'. Error: docker: Error // response from daemon: OCI runtime create failed: container_linux.go:367: starting container process caused: chdir to cwd ("/home/jenkins/build/workspace/restc-staging") set in config.json failed: permission denied: unknown.        
//                  stage('Ubuntu Jammy') {
//                     agent {
//                         dockerfile {
//                             filename 'Dockefile.ubuntu-jammy'
//                             dir 'ci/jenkins'
//                             label 'docker'
//                         }
//                     }
//                     
//                     options {
//                         timeout(time: 30, unit: "MINUTES")
//                     }
// 
//                     steps {
//                         echo "Building on ubuntu-jammy-AMD64 in ${WORKSPACE}"
//                         checkout scm
//                         sh 'pwd; ls -la'
//                         sh 'rm -rf build'
//                         sh 'mkdir build'
//                         sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release -DRESTC_CPP_USE_CPP17=ON .. && make -j $(nproc)'
// 
//                         echo 'Getting ready to run tests'
//                         script {
//                             try {
//                                 sh 'cd build && ctest --no-compress-output -T Test'
//                             } catch (exc) {
//                                 
//                                 unstable(message: "${STAGE_NAME} - Testing failed")
//                             }
//                         }
//                     }
//                 }
//                 
//                 stage('Ubuntu Jammy MT CTX') {
//                     agent {
//                         dockerfile {
//                             filename 'Dockefile.ubuntu-jammy'
//                             dir 'ci/jenkins'
//                             label 'docker'
//                         }
//                     }
//                     
//                     options {
//                         timeout(time: 30, unit: "MINUTES")
//                     }
// 
//                     steps {
//                         echo "Building on ubuntu-jammy-AMD64 in ${WORKSPACE}"
//                         checkout scm
//                         sh 'pwd; ls -la'
//                         sh 'rm -rf build'
//                         sh 'mkdir build'
//                         sh 'cd build && cmake -DRESTC_CPP_THREADED_CTX=ON -DCMAKE_BUILD_TYPE=Release -DRESTC_CPP_USE_CPP17=ON .. && make -j $(nproc)'
// 
//                         echo 'Getting ready to run tests'
//                         script {
//                             try {
//                                 sh 'cd build && ctest --no-compress-output -T Test'
//                             } catch (exc) {
//                                 
//                                 unstable(message: "${STAGE_NAME} - Testing failed")
//                             }
//                         }
//                     }
//                 }
                                
                stage('Ubuntu Xenial') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.ubuntu-xenial'
                            dir 'ci/jenkins'
                            label 'docker'
                        }
                    }
                    
                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on ubuntu-xenial-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DGTEST_TAG=release-1.10.0 -DCMAKE_BUILD_TYPE=Release -DRESTC_CPP_USE_CPP17=OFF .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest -E "HTTPS_FUNCTIONAL_TESTS|PROXY_TESTS" --no-compress-output -T Test'
                            } catch (exc) {
                                
                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }
                
                stage('Ubuntu Xenial MT CTX') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.ubuntu-xenial'
                            dir 'ci/jenkins'
                            label 'docker'
                        }
                    }
                    
                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on ubuntu-xenial-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DGTEST_TAG=release-1.10.0 -DRESTC_CPP_THREADED_CTX=ON -DCMAKE_BUILD_TYPE=Release -DRESTC_CPP_USE_CPP17=OFF .. && make -j $(nproc)'

                        echo 'Getting ready to run tests'
                        script {
                            try {
                                sh 'cd build && ctest -E "HTTPS_FUNCTIONAL_TESTS|PROXY_TESTS" --no-compress-output -T Test'
                            } catch (exc) {
                                
                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }
                
                stage('Debian Buster C++17') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-buster'
                            dir 'ci/jenkins'
                            label 'docker'
                        }
                    }
                    
                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on debian-buster-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release -DRESTC_CPP_USE_CPP17=ON .. && make -j $(nproc)'

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
                
                stage('Debian Buster C++17 MT CTX') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-buster'
                            dir 'ci/jenkins'
                            label 'docker'
                        }
                    }
                    
                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

                    steps {
                        echo "Building on debian-buster-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DRESTC_CPP_THREADED_CTX=ON -DCMAKE_BUILD_TYPE=Release -DRESTC_CPP_USE_CPP17=ON .. && make -j $(nproc)'

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
                
                stage('Debian Bullseye C++17') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-bullseye'
                            dir 'ci/jenkins'
                            label 'docker'
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
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release -DRESTC_CPP_USE_CPP17=ON .. && make -j $(nproc)'

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
                
                 stage('Debian Bullseye C++17 MT CTX') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-bullseye'
                            dir 'ci/jenkins'
                            label 'docker'
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
                        sh 'cd build && cmake -DRESTC_CPP_THREADED_CTX=ON  -DCMAKE_BUILD_TYPE=Release -DRESTC_CPP_USE_CPP17=ON .. && make -j $(nproc)'

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

                stage('Debian Bookworm, C++17') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-bookworm'
                            dir 'ci/jenkins'
                            label 'docker'
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
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release -DRESTC_CPP_USE_CPP17=ON .. && make -j $(nproc)'

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

                 stage('Debian Bookworm MT CTX C++17') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-bookworm'
                            dir 'ci/jenkins'
                            label 'docker'
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
                        sh 'cd build && cmake -DRESTC_CPP_THREADED_CTX=ON -DCMAKE_BUILD_TYPE=Release -DRESTC_CPP_USE_CPP17=ON .. && make -j $(nproc)'

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

                stage('Debian Testing C++17') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-testing'
                            dir 'ci/jenkins'
                            label 'docker'
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
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release -DRESTC_CPP_USE_CPP17=ON .. && make -j $(nproc)'

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
                
                stage('Debian Testing MT CTX C++17') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-testing'
                            dir 'ci/jenkins'
                            label 'docker'
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
                        sh 'cd build && cmake -DRESTC_CPP_THREADED_CTX=ON -DCMAKE_BUILD_TYPE=Release -DRESTC_CPP_USE_CPP17=ON .. && make -j $(nproc)'

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
//                                 
//                                 unstable(message: "${STAGE_NAME} - Testing failed")
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
//                                 
//                                 unstable(message: "${STAGE_NAME} - Testing failed")
//                             }
//                         }
//                     }
//                 }

                stage('Windows X64 with vcpkg C++17') {

                    agent {label 'windows'}
                    
                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

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
                                
                                unstable(message: "${STAGE_NAME} - Testing failed")
                            }
                        }
                    }
                }
                
                stage('Windows X64 with vcpkg MT CTX C++17') {

                    agent {label 'windows'}
                    
                    options {
                        timeout(time: 30, unit: "MINUTES")
                    }

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
                            cmake -DRESTC_CPP_THREADED_CTX=ON -DRESTC_CPP_USE_CPP17=ON -DCMAKE_PREFIX_PATH=C:\\devel\\vcpkg\\installed\\x64-windows\\lib;C:\\devel\\vcpkg\\installed\\x64-windows\\include ..
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
                                
                                unstable(message: "${STAGE_NAME} - Testing failed")
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

