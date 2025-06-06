
import React from 'react';
import { FileNode } from '../types';
import { FolderIcon, DocumentTextIcon } from '@heroicons/react/24/outline';


interface FileTreeProps {
  nodes: FileNode[];
}

const FileItem: React.FC<{ node: FileNode; level?: number }> = ({ node, level = 0 }) => {
  const isDirectory = node.type === 'directory';
  const Icon = isDirectory ? FolderIcon : DocumentTextIcon;

  return (
    <li style={{ paddingLeft: `${level * 1.5}rem` }} className="mb-1">
      <div className="flex items-center space-x-2 text-text-secondary hover:text-primary transition-colors duration-200">
        <Icon className="w-5 h-5 flex-shrink-0 text-accent" />
        <span className={`font-medium ${isDirectory ? 'text-primary' : ''}`}>{node.name}</span>
        {node.comment && <span className="text-sm text-gray-500 italic"># {node.comment}</span>}
      </div>
      {isDirectory && node.children && node.children.length > 0 && (
        <ul className="mt-1">
          {node.children.map((child, index) => (
            <FileItem key={index} node={child} level={level + 1} />
          ))}
        </ul>
      )}
    </li>
  );
};

const FileTree: React.FC<FileTreeProps> = ({ nodes }) => {
  return (
    <ul className="space-y-1 p-4 bg-gray-50 rounded-md shadow">
      {nodes.map((node, index) => (
        <FileItem key={index} node={node} />
      ))}
    </ul>
  );
};

export default FileTree;
