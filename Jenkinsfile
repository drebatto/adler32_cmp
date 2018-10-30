pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh 'make'
      }
    }
    stage('') {
      steps {
        archiveArtifacts 'adler32_cmp'
      }
    }
  }
}