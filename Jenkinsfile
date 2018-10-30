pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh '/usr/bin/make'
      }
    }
    stage('error') {
      steps {
        archiveArtifacts 'adler32_cmp'
      }
    }
  }
}