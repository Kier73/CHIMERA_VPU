
export interface FileNode {
  name: string;
  type: 'file' | 'directory';
  comment?: string;
  children?: FileNode[];
}

export interface PillarData {
  id: string;
  title: string;
  alias?: string;
  description: string | React.ReactNode;
  architecture?: string | React.ReactNode;
  dataFlow?: string | React.ReactNode;
  coreStructures?: string | React.ReactNode;
  codeExamples?: { title: string; language: string; code: string }[];
  subSections?: { title: string; content: string | React.ReactNode }[];
}

export interface ProjectDataType {
  title: string;
  introduction: string;
  repoStructure: FileNode[];
  cmakeFileContent: string;
  e2eTestContent: string;
  pillars: PillarData[];
  vpuTechSpec: {
    title: string;
    coreDataStructures: { name: string; definition: string; language: string }[];
  };
  pillar2Implementation: { title: string, code: string, language: string, exampleUsage: string };
}
